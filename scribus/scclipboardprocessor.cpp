#include "scclipboardprocessor.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QGuiApplication>
#include <QMimeData>
#include <QString>
#include <QRegularExpression>

#include <QTextStream>
#include <QXmlStreamReader>

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

#include "pageitem.h"
#include "prefsmanager.h"
#include "scfonts.h"
#include "scribusdoc.h"
#include "text/specialchars.h"
#include "undomanager.h"


ScClipboardProcessor::ScClipboardProcessor()
{
}

ScClipboardProcessor::ScClipboardProcessor(ScribusDoc* doc, PageItem *pageItem)
{
	m_doc = doc;
	m_pageItem = pageItem;
}

void ScClipboardProcessor::setContent(const QString &content, ContentType type)
{
	m_content = content;
	m_contentType = type;
	processed = false;
}

void ScClipboardProcessor::setDocAndPageItem(ScribusDoc *doc, PageItem *pageItem)
{
	m_doc = doc;
	m_pageItem = pageItem;
}

bool ScClipboardProcessor::process()
{
#ifdef SCCLIP_DEBUG
	QString dashes(40, '-');  // Create a string of 'count' dashes
	qDebug() << dashes;
	dumpClipboardData();
	qDebug() << "Content Type:"<< m_contentType;
	qDebug() << dashes;
#endif
	if (m_content.isEmpty())
	{
		qDebug()<<"ScClipboardProcessor: Content is empty";
		return false;
	}
	switch (m_contentType)
	{
		case ContentType::Text:
			return processText();
			break;
		case ContentType::HTML:
			return processHTML();
			break;
		default:
			break;
	}
	return false;
}

bool ScClipboardProcessor::processText()
{
	QString text(m_content);
	text.replace("\r\n", SpecialChars::PARSEP);
	text.replace('\n', SpecialChars::PARSEP);
	m_result = text;
	processed = true;
	UndoManager* undoManager = UndoManager::instance();
	if (UndoManager::undoEnabled())
	{
		auto *is = new SimpleState(Um::Paste, QString(), Um::IPaste);
		is->set("PASTE_PLAINTEXT");
		is->set("START", m_pageItem->itemText.cursorPosition());
		is->set("TEXT", m_result);
		undoManager->action(m_pageItem, is);
	}
	m_pageItem->itemText.insertChars(m_result, true);

	return true;
}

bool ScClipboardProcessor::processHTML()
{
	//Something from Apple Pages (just a first example)
	if (m_content.contains("Cocoa HTML Writer"))
		return html_Cocoa_Process();

	//Something from Microsoft Word (just a first example, maybe all of Microsoft's apps)
	if (m_content.contains("schemas-microsoft-com"))
		return html_MSFT_Process();

	return processHTML_Other();
}

bool ScClipboardProcessor::html_MSFT_Process()
{
	qDebug()<<Q_FUNC_INFO;
	// Convert to a const xmlChar*, parse with libxml2
	const xmlChar* html_content_cstr = reinterpret_cast<const xmlChar*>(m_content.toUtf8().constData());

	// Parse the HTML content using libxml2's HTML parser
	htmlDocPtr doc = htmlReadDoc(html_content_cstr, NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

	if (!doc)
	{
		qDebug() << "Failed to parse the HTML content.";
		return false;
	}

	// Get the document's root <html> tag
	xmlNodePtr root = xmlDocGetRootElement(doc);
	if (root != nullptr)
	{
		// Parse the content
		html_MSFT_Parse(root);
	}
	// Clean up the document after parsing
	xmlFreeDoc(doc);
	xmlCleanupParser();

	processed = true;
	return true;
}

bool ScClipboardProcessor::styleToProcess(const QString &style)
{
	return	style.contains("color") ||
			style.contains("font-family:") ||
			style.contains("font-size") ||
			style.contains("font-style:") ||
			style.contains("font-weight:") ||
			style.contains("text-decoration:");
}

void ScClipboardProcessor::html_MSFT_Parse(xmlNode *node)
{
	if (!node)
		return;

	// Find <style> in the <head> area and parse styles
	for (xmlNode *cur = node->children; cur; cur = cur->next)
	{
		if (cur->type == XML_ELEMENT_NODE && xmlStrcmp(cur->name, (const xmlChar *)"head") == 0)
		{
			for (xmlNode *headChild = cur->children; headChild; headChild = headChild->next)
			{
				msftParseStyles(headChild, cssStyles);
			}
		}
	}

	processMSFTCSS(cssStyles);

	for (xmlNode *cur = node->children; cur; cur = cur->next)
	{
		if (cur->type == XML_ELEMENT_NODE && xmlStrcmp(cur->name, (const xmlChar *)"body") == 0)
		{
			msftParseParagraphs(cur->children, cssStyles);
		}
	}
}

// Function to extract styles from <style>s in <head>
void ScClipboardProcessor::msftParseStyles(xmlNode *node, QMap<QString, QString> &styles)
{
	if (node->type == XML_ELEMENT_NODE && xmlStrcasecmp(node->name, (const xmlChar *)"style") == 0)
	{
		xmlChar *styleContent = xmlNodeGetContent(node);
		QString stylesText = QString::fromUtf8((const char*)styleContent);
		xmlFree(styleContent);

		// // Regular expression to extract class and styles
		static QRegularExpression regex("\\.(\\w+)\\s*\\{([^}]*)\\}");
		QRegularExpressionMatchIterator it = regex.globalMatch(stylesText);
		static QRegularExpression regex2("mso-[^:]+:[^;]+;?");
		static QRegularExpression regex3("[\t\r\n]");

		while (it.hasNext())
		{
			QRegularExpressionMatch match = it.next();
			if (match.hasMatch())
			{
				QString className = match.captured(1).trimmed();
				QString style = match.captured(2).trimmed();
				style.remove(regex2);
				style.remove(regex3);
				if (styleToProcess(style))
					styles[className] = style;
			}
		}
	}
}

void ScClipboardProcessor::processMSFTCSS(const QMap<QString, QString> &styles)
{
	QMapIterator<QString, QString> i(styles);
	while (i.hasNext())
	{
		i.next();
		QString paraStyleName = i.key();

		// Parse properties
		QMap<QString, QString> propertyMap;
		QStringList propertyList = i.value().split(';', Qt::SkipEmptyParts);
		for (const QString& property : std::as_const(propertyList))
		{
			QStringList keyValue = property.split(':', Qt::SkipEmptyParts);
			if (keyValue.size() == 2)
				propertyMap.insert(keyValue[0].trimmed().toLower(), keyValue[1].trimmed());
		}
		ParagraphStyle newParaStyle;
		newParaStyle.setDefaultStyle(false);
		newParaStyle.setName(paraStyleName);
		for (auto it = propertyMap.begin(); it != propertyMap.end(); ++it)
		{
			if (it.key() == "color")
			{
				ScColor newColor;
				newColor.fromQColor(QColor(it.value()));
				QString colorName = m_doc->PageColors.tryAddColor("FromCopy"+it.value(), newColor);
				newParaStyle.charStyle().setFillColor(colorName);
			}
			if (it.key() == "font-family")
			{
				static QRegularExpression regex(R"(\s*\"?([^\",]+)\"?\s*,?)");
				QRegularExpressionMatch match = regex.match(it.value());
				QString fontName;
				if (match.hasMatch())
					fontName = match.captured(1);
				//else
					// qDebug() << "No match found";
				//Process font name
				SCFonts& availableFonts = PrefsManager::instance().appPrefs.fontPrefs.AvailFonts;
				if (availableFonts.fontMap.contains(fontName))
				{
					QStringList styles = availableFonts.fontMap[fontName];
					QString style;
					if (!styles.isEmpty())
						style = styles[0];
					if (styles.contains("Regular"))
						style = "Regular";
					else if (styles.contains("Roman"))
						style = "Roman";
					else if (styles.contains("Medium"))
						style = "Medium";
					else if (styles.contains("Book"))
						style = "Book";
					const ScFace& face = availableFonts.findFont(fontName, style);
					QString textFont;
					if (face != ScFace::none())
						textFont = face.family() + " " + face.style();
					else
						textFont = PrefsManager::instance().appPrefs.itemToolPrefs.textFont;
					newParaStyle.charStyle().setFont((*m_doc->AllFonts)[textFont]);
				}
			}
			if (it.key() == "font-size")
			{
				static QRegularExpression regex(R"(\s*([\d.]+)pt)");
				QRegularExpressionMatch match = regex.match(it.value());
				double fontSize;
				if (match.hasMatch())
					fontSize = match.captured(1).toDouble() * 10.0;
				else
					fontSize = PrefsManager::instance().appPrefs.itemToolPrefs.textSize;
				newParaStyle.charStyle().setFontSize(fontSize);
			}
			StyleSet<ParagraphStyle> tmpParagraphStyleSet;
			tmpParagraphStyleSet.create(newParaStyle);
			m_doc->redefineStyles(tmpParagraphStyleSet, false);
		}
	}
}

//Function to extract text content (including formatted text like <b> and <i>)
QString ScClipboardProcessor::msftExtractText(xmlNode *node, QList<TextSegment> &segments, bool bold, bool italic, bool underline, QString color, double fontsize, QString family)
{
	QString text;
	for (xmlNode *cur = node; cur; cur = cur->next)
	{
		if (cur->type == XML_TEXT_NODE)
		{
			QString t = QString::fromUtf8((const char *)cur->content);
			// qDebug()<<"XML_TEXT_NODE"<<t;
			text += t;
			if (!text.isEmpty())
			{
				// qDebug()<<"<<"<<t<<">>";
				t.replace("\r\n", "");
				t.replace('\n', "");
				segments.append({t, color, bold, italic, underline, fontsize, family});
			}
		}
		else if (cur->type == XML_ELEMENT_NODE)
		{
			QString tag = QString::fromUtf8((const char *)cur->name);
			// qDebug()<<"XML_ELEMENT_NODE"<<tag;
			bool newBold = bold || (tag == "b");
			bool newItalic = italic || (tag == "i");
			bool newUnderline = underline || (tag == "u");
			QString newColor;
			double newFontSize = 0.0;
			QString newFamily;
			if (tag == "span")
			{
				// qDebug() << "-----------------------------------";
				xmlChar *styleAttr = xmlGetProp(cur, (const xmlChar *)"style");
				QString styleData = styleAttr ? QString::fromUtf8((const char*)styleAttr) : "None";
				xmlFree(styleAttr);
				static QRegularExpression regexMso("mso-[^:]+:[^;]+;?");
				static QRegularExpression regexColor("color:(.*);");
				static QRegularExpression regexFontsize(R"(font-size:([\d.]+)pt)");
				static QRegularExpression regexFamily(R"(font-family:(\"([^']+)\"|([\w]+)))");
				styleData.remove(regexMso);
				QRegularExpressionMatch matchC = regexColor.match(styleData);
				if (matchC.hasMatch())
					newColor = matchC.captured(1);
				QRegularExpressionMatch matchFS = regexFontsize.match(styleData);
				if (matchFS.hasMatch())
					newFontSize = matchFS.captured(1).toDouble() * 10.0;
				QRegularExpressionMatch matchFamily = regexFamily.match(styleData);
				if (matchFamily.hasMatch())
					newFamily = matchFamily.captured(matchFamily.lastCapturedIndex());
			}


			QString innerText = msftExtractText(cur->children, segments, newBold, newItalic, newUnderline, newColor, newFontSize, newFamily);
			// if (tag == "b")
			// {
			// 	text += "<b>" + innerText + "</b>";
			// }
			// else if (tag == "i")
			// {
			// 	text += "<i>" + innerText + "</i>";
			// }
			// else if (tag == "u")
			// {
			// 	text += "<u>" + innerText + "</u>";
			// }
			// else if (tag == "span")
			// {
			// 	qDebug() << "-----------------------------------";
			// 	xmlChar *styleAttr = xmlGetProp(cur, (const xmlChar *)"style");
			// 	QString styleData = styleAttr ? QString::fromUtf8((const char*)styleAttr) : "None";
			// 	xmlFree(styleAttr);
			// 	static QRegularExpression regex("mso-[^:]+:[^;]+;?");
			// 	styleData.remove(regex);
			// 	qDebug()<<"Style in para:"<<styleData;
			// 	text += innerText;
			// }
			// else
			// {
				text += innerText;
			// }
		}
	}
	return text;
}

void ScClipboardProcessor::msftParseParagraphs(xmlNode *node, QMap<QString, QString> &styles)
{
	SCFonts& availableFonts = PrefsManager::instance().appPrefs.fontPrefs.AvailFonts;
	QList<TextSegment> segments;
	for (xmlNode *cur = node; cur; cur = cur->next)
	{
		// qDebug() << QString::fromUtf8((const char *)cur->name);
		if (cur->type == XML_ELEMENT_NODE && xmlStrcmp(cur->name, (const xmlChar *)"span") == 0)
		{
			// qDebug() << "-----------------------------------";
			xmlChar *styleAttr = xmlGetProp(cur, (const xmlChar *)"style");
			QString className = styleAttr ? QString::fromUtf8((const char*)styleAttr) : "None";
			// qDebug()<<"Style in para:"<<className;
			xmlFree(styleAttr);
		}
		if (cur->type == XML_ELEMENT_NODE && xmlStrcmp(cur->name, (const xmlChar *)"p") == 0)
		{
			xmlChar *classAttr = xmlGetProp(cur, (const xmlChar *)"class");
			QString className = classAttr ? QString::fromUtf8((const char*)classAttr) : "None";
			xmlFree(classAttr);

			QString content;
			content = msftExtractText(cur->children, segments);
			// qDebug() << "-----------------------------------";
			// qDebug() << "Paragraph Class:" << className;
			// qDebug() << "Text Content:" << content;

			ParagraphStyle currPstyle = m_doc->paragraphStyle(className);
			int pos = qMax(0, m_pageItem->itemText.cursorPosition());

			for (const auto &segment : std::as_const(segments))
			{
				QString style;
				QString currFamily(segment.family.isEmpty() ? currPstyle.charStyle().font().family() : segment.family);
				if (!segment.isBold && !segment.isItalic)
				{
					QStringList styles = availableFonts.fontMap[currFamily];
					QString style;
					if (!styles.isEmpty())
						style = styles[0];
					if (styles.contains("Regular"))
						style = "Regular";
					else if (styles.contains("Roman"))
						style = "Roman";
					else if (styles.contains("Medium"))
						style = "Medium";
					else if (styles.contains("Book"))
						style = "Book";
				}
				else
				{
					if(segment.isBold && !segment.isItalic)
						style = "Bold";
					if (!segment.isBold && segment.isItalic)
						style = "Italic";
					if(segment.isBold && segment.isItalic)
						style = "Bold Italic";
				}
				if (segment.hasUnderline)
				{
					int s = 0;
					s |= 8;
					currPstyle.charStyle().setFeatures(static_cast<StyleFlag>(s).featureList());
				}
				const ScFace& face = availableFonts.findFont(currFamily, style);
				if (!segment.color.isEmpty())
				{
					ScColor newColor;
					newColor.fromQColor(QColor(segment.color));
					QString colorName = m_doc->PageColors.tryAddColor("FromCopy"+segment.color, newColor);
					currPstyle.charStyle().setFillColor(colorName);
				}
				if (segment.fontsize > 0.0)
					currPstyle.charStyle().setFontSize(segment.fontsize);
				if (face != ScFace::none())
				{
					// qDebug()<<"some bold/italic"<<content;
					currPstyle.charStyle().setFont(face);
					m_pageItem->itemText.insertChars(pos, segment.text);
					m_pageItem->itemText.applyStyle(pos, currPstyle);
					m_pageItem->itemText.applyCharStyle(pos, segment.text.length(), currPstyle.charStyle());
				}
				else
					qDebug()<<"No face found";
				pos = m_pageItem->itemText.cursorPosition();
			}
			segments.clear();
			content.clear();
			//Add a new line after the paragraph
			pos = qMax(0, m_pageItem->itemText.cursorPosition());
			m_pageItem->itemText.insertChars(pos, SpecialChars::PARSEP);
			m_pageItem->itemText.applyStyle(pos, currPstyle);
		}
	}
}

bool ScClipboardProcessor::html_Cocoa_Process()
{
	qDebug()<<Q_FUNC_INFO;
	// Convert to a const xmlChar*, parse with libxml2
	const xmlChar* html_content_cstr = reinterpret_cast<const xmlChar*>(m_content.toUtf8().constData());

	// Parse the HTML content using libxml2's HTML parser
	htmlDocPtr doc = htmlReadDoc(html_content_cstr, NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

	if (!doc)
	{
		qDebug() << "Failed to parse the HTML content.";
		return false;
	}

	// Get the document's root <html> tag
	xmlNodePtr root = xmlDocGetRootElement(doc);
	if (root != nullptr)
	{
		// Parse the content
		html_Cocoa_Parse(root);
	}
	// Clean up the document after parsing
	xmlFreeDoc(doc);
	xmlCleanupParser();

	processed = true;
	return true;
}

void ScClipboardProcessor::html_Cocoa_Parse(xmlNodePtr node)
{
	if (!node)
		return;

	// Find <style> in the <head> area and parse styles
	for (xmlNode *cur = node->children; cur; cur = cur->next)
	{
		if (cur->type == XML_ELEMENT_NODE && xmlStrcmp(cur->name, (const xmlChar *)"head") == 0)
		{
			for (xmlNode *headChild = cur->children; headChild; headChild = headChild->next)
			{
				cocoaParseStyles(headChild, cssStyles);
			}
		}
	}

	processCocoaCSS(cssStyles);

	// Find <p> elements and extract text
	for (xmlNode *cur = node->children; cur; cur = cur->next)
	{
		if (cur->type == XML_ELEMENT_NODE && xmlStrcmp(cur->name, (const xmlChar *)"body") == 0)
		{
			cocoaParseParagraphs(cur->children, cssStyles);
		}
	}
}

// Function to extract styles from <style>s in <head>
void ScClipboardProcessor::cocoaParseStyles(xmlNode *node, QMap<QString, QString> &styles)
{
	if (node->type == XML_ELEMENT_NODE && xmlStrcmp(node->name, (const xmlChar *)"style") == 0)
	{
		xmlChar *styleContent = xmlNodeGetContent(node);
		QString stylesText = QString::fromUtf8((const char*)styleContent);
		xmlFree(styleContent);

		// Regular expression to extract class and styles
		static QRegularExpression regex(R"(p\.(\w+)\s*\{(.*?)\})");
		QRegularExpressionMatchIterator it = regex.globalMatch(stylesText);

		while (it.hasNext())
		{
			QRegularExpressionMatch match = it.next();
			if (match.hasMatch())
			{
				QString className = match.captured(1).trimmed();
				QString style = match.captured(2).trimmed();
				styles[className] = style;
			}
		}
	}
}

void ScClipboardProcessor::processCocoaCSS(const QMap<QString, QString> &styles)
{
	QMapIterator<QString, QString> i(styles);
	while (i.hasNext())
	{
		i.next();
		QString paraStyleName = i.key();

		// Parse properties
		QMap<QString, QString> propertyMap;
		QStringList propertyList = i.value().split(';', Qt::SkipEmptyParts);
		for (const QString& property : std::as_const(propertyList))
		{
			QStringList keyValue = property.split(':', Qt::SkipEmptyParts);
			if (keyValue.size() == 2)
				propertyMap.insert(keyValue[0].trimmed().toLower(), keyValue[1].trimmed());
		}
		ParagraphStyle newParaStyle;
		newParaStyle.setDefaultStyle(false);
		newParaStyle.setName(paraStyleName);
		//Clearly this regex only deals with this format of CSS:
		//p.p1 {margin: 0.0px 0.0px 12.0px 0.0px; font: 11.0px 'Helvetica Neue'; color: #1a1a1a}
		for (auto it = propertyMap.begin(); it != propertyMap.end(); ++it)
		{
			if (it.key() == "color")
			{
				ScColor newColor;
				newColor.fromQColor(QColor(it.value()));
				QString colorName = m_doc->PageColors.tryAddColor("FromCopy"+it.value(), newColor);
				newParaStyle.charStyle().setFillColor(colorName);
			}
			if (it.key() == "font")
			{
				// static QRegularExpression regex(R"(\s*([\d.]+)px\s*'([^']+)')");
				static QRegularExpression regex(R"(\s*([\d.]+)px\s*('([^']+)'|([\w]+)))");

				QRegularExpressionMatch match = regex.match(it.value());
				QString fontName;
				double fontSize;
				if (match.hasMatch())
				{
					//Font name moves between capture 3 and 4 depending on if quotes are used
					fontSize = match.captured(1).toDouble() * 10.0;
					fontName = match.captured(match.lastCapturedIndex());
				}
				else
				{
					fontSize = PrefsManager::instance().appPrefs.itemToolPrefs.textSize;
					// qDebug() << "No match found";
				}
				//Process font name
				SCFonts& availableFonts = PrefsManager::instance().appPrefs.fontPrefs.AvailFonts;
				if (availableFonts.fontMap.contains(fontName))
				{
					QStringList styles = availableFonts.fontMap[fontName];
					QString style;
					if (!styles.isEmpty())
						style = styles[0];
					if (styles.contains("Regular"))
						style = "Regular";
					else if (styles.contains("Roman"))
						style = "Roman";
					else if (styles.contains("Medium"))
						style = "Medium";
					else if (styles.contains("Book"))
						style = "Book";
					const ScFace& face = availableFonts.findFont(fontName, style);
					QString textFont;
					if (face != ScFace::none())
						textFont = face.family() + " " + face.style();
					else
						textFont = PrefsManager::instance().appPrefs.itemToolPrefs.textFont;
					newParaStyle.charStyle().setFont((*m_doc->AllFonts)[textFont]);
					newParaStyle.charStyle().setFontSize(fontSize);
				}
			}
			StyleSet<ParagraphStyle> tmpParagraphStyleSet;
			tmpParagraphStyleSet.create(newParaStyle);
			m_doc->redefineStyles(tmpParagraphStyleSet, false);
		}
	}
}

// Function to extract text content (including formatted text like <b> and <i>)
QString ScClipboardProcessor::cocoaExtractText(xmlNode *node, QList<TextSegment> &segments, bool bold, bool italic, bool underline)
{
	QString text;
	for (xmlNode *cur = node; cur; cur = cur->next)
	{
		if (cur->type == XML_TEXT_NODE)
		{
			QString t = QString::fromUtf8((const char *)cur->content);
			text += t;
			if (!text.isEmpty())
				segments.append({t, QString(), bold, italic, underline, 0.0});
		}
		else if (cur->type == XML_ELEMENT_NODE)
		{
			QString tag = QString::fromUtf8((const char *)cur->name);
			bool newBold = bold || (tag == "b");
			bool newItalic = italic || (tag == "i");
			//unlikely this will work as underline is done with a span in Cocoa HTML
			bool newUnderline = underline || (tag == "u");
			QString innerText = cocoaExtractText(cur->children, segments, newBold, newItalic, newUnderline);

			// if (tag == "b")
			// {
			// 	text += "<b>" + innerText + "</b>";
			// }
			// else if (tag == "i")
			// {
			// 	text += "<i>" + innerText + "</i>";
			// }
			// else if (tag == "u")
			// {
			// 	text += "<u>" + innerText + "</u>";
			// }
			// else
			// {
				text += innerText;
			// }
		}
	}
	return text;
}

// Function to parse paragraphs and their styles
void ScClipboardProcessor::cocoaParseParagraphs(xmlNode *node, QMap<QString, QString> &styles)
{
	SCFonts& availableFonts = PrefsManager::instance().appPrefs.fontPrefs.AvailFonts;
	QList<TextSegment> segments;
	for (xmlNode *cur = node; cur; cur = cur->next)
	{
		if (cur->type == XML_ELEMENT_NODE && xmlStrcmp(cur->name, (const xmlChar *)"p") == 0)
		{
			xmlChar *classAttr = xmlGetProp(cur, (const xmlChar *)"class");
			QString className = classAttr ? QString::fromUtf8((const char*)classAttr) : "None";
			xmlFree(classAttr);

			QString content = cocoaExtractText(cur->children, segments);
			// qDebug() << "-----------------------------------";
			// qDebug() << "Paragraph Class:" << className;
			// qDebug() << "Style:" << style;
			// qDebug() << "Text Content:" << content;

			ParagraphStyle currPstyle = m_doc->paragraphStyle(className);
			int pos = qMax(0, m_pageItem->itemText.cursorPosition());
			for (const auto &segment : std::as_const(segments))
			{
				QString style;
				QString currFamily(currPstyle.charStyle().font().family());
				if (!segment.isBold && !segment.isItalic)
				{
					QStringList styles = availableFonts.fontMap[currFamily];
					QString style;
					if (!styles.isEmpty())
						style = styles[0];
					if (styles.contains("Regular"))
						style = "Regular";
					else if (styles.contains("Roman"))
						style = "Roman";
					else if (styles.contains("Medium"))
						style = "Medium";
					else if (styles.contains("Book"))
						style = "Book";
				}
				else
				{
					if(segment.isBold && !segment.isItalic)
						style = "Bold";
					if (!segment.isBold && segment.isItalic)
						style = "Italic";
					if(segment.isBold && segment.isItalic)
						style = "Bold Italic";
				}
				if (segment.hasUnderline)
				{
					int s = 0;
					s |= 8;
					currPstyle.charStyle().setFeatures(static_cast<StyleFlag>(s).featureList());
				}
				const ScFace& face = availableFonts.findFont(currFamily, style);
				if (face != ScFace::none())
				{
					currPstyle.charStyle().setFont(face);
					m_pageItem->itemText.insertChars(pos, segment.text);
					m_pageItem->itemText.applyStyle(pos, currPstyle);
					m_pageItem->itemText.applyCharStyle(pos, segment.text.length(), currPstyle.charStyle());
				}
				else
					qDebug()<<"No face found";
				pos = m_pageItem->itemText.cursorPosition();
			}
			segments.clear();
			content.clear();
			//Add a new line after the paragraph
			pos = qMax(0, m_pageItem->itemText.cursorPosition());
			m_pageItem->itemText.insertChars(pos, SpecialChars::PARSEP);
			m_pageItem->itemText.applyStyle(pos, currPstyle);
		}
	}
}

bool ScClipboardProcessor::processHTML_Other()
{
	QTextDocument qTextDoc;
	qTextDoc.setHtml(m_content);

	//temporary result for 1.7.0
	m_result = qTextDoc.toPlainText();
	if (m_result.isEmpty())
	{
		qTextDoc.setPlainText(m_content);
		m_result = qTextDoc.toPlainText();
	}
	m_result.replace("\r\n", SpecialChars::PARSEP);
	m_result.replace('\n', SpecialChars::PARSEP);
	m_pageItem->itemText.insertChars(m_result, true);
	processed = true;
	return true;
}

const QString& ScClipboardProcessor::data()
{
	if (!processed)
		qDebug()<<"ScClipboardProcessor: Data not processed";
	return m_result;
}

void ScClipboardProcessor::reset()
{
	m_content.clear();
	m_contentType = ContentType::Unknown;
	m_result.clear();
	cssStyles.clear();
	m_doc = nullptr;
	m_pageItem = nullptr;
	processed = false;
}

void ScClipboardProcessor::dumpClipboardData()
{
	qDebug()<<QApplication::clipboard()->mimeData()->formats();
	qDebug()<<QApplication::clipboard()->mimeData()->text();
	QDebug debug = qDebug();
	// debug.noquote()<<QApplication::clipboard()->mimeData()->html();
	debug<<QApplication::clipboard()->mimeData()->html();
}
