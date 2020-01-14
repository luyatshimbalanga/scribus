%global	build_repo https://github.com/scribusproject/scribus/
%global	build_branch master
%global	version_string 1.5.6
%define	build_commit %(git ls-remote %{build_repo} | grep "refs/heads/%{build_branch}" | cut -c1-41)
%define	build_shortcommit %(c=%{build_commit}; echo ${c:0:7})
%global	build_timestamp %(date +"%Y%m%d")
%global	rel_build %{build_timestamp}git%{build_shortcommit}%{?dist}

Name:		scribus
Version:	%{version_string}
Release:	0.1.%{rel_build}
Summary:	Open Source Page Layout
License:	GPLv2+
URL:		http://www.scribus.net/
# svn export svn://scribus.net/trunk/Scribus scribus
# tar --exclude-vcs -cJf scribus-1.5.0-20161204svn21568.tar.xz scribus
Source0:	%{build_repo}/archive/%{build_branch}.tar.gz

BuildRequires:	boost-devel
BuildRequires:	cmake
BuildRequires:	cups-devel
BuildRequires:	desktop-file-utils
BuildRequires:	gcc-c++
BuildRequires:	ghostscript
BuildRequires:	hyphen-devel
BuildRequires:	libappstream-glib
BuildRequires:	libcdr-devel
BuildRequires:	libfreehand-devel
BuildRequires:	libpagemaker-devel
BuildRequires:	libqxp-devel
BuildRequires:	librevenge-devel
BuildRequires:	libwpd-devel
BuildRequires:	libwpg-devel
BuildRequires:	libzmf-devel
BuildRequires:	pkgconfig(cairo)
BuildRequires:	pkgconfig(fontconfig)
BuildRequires:	pkgconfig(freetype2)
BuildRequires:	pkgconfig(gnutls)
BuildRequires:	pkgconfig(GraphicsMagick)
BuildRequires:	pkgconfig(GraphicsMagick++)
BuildRequires:	pkgconfig(harfbuzz)
BuildRequires:	pkgconfig(hunspell)
BuildRequires:	pkgconfig(icu-uc)
BuildRequires:	pkgconfig(lcms2)
BuildRequires:	pkgconfig(libjpeg)
BuildRequires:	pkgconfig(libpng)
BuildRequires:	pkgconfig(libpodofo)
BuildRequires:	pkgconfig(libtiff-4)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:	pkgconfig(openscenegraph)
BuildRequires:	pkgconfig(openssl)
BuildRequires:  pkgconfig(poppler)
BuildRequires:  pkgconfig(poppler-cpp)
BuildRequires:  pkgconfig(poppler-data)
BuildRequires:	pkgconfig(python3)
BuildRequires:	pkgconfig(Qt5)
BuildRequires:  pkgconifg(Qt5Declarative)
BuildRequires:  pkgconfig(Qt5UiTools)
BuildRequires:	pkgconfig(Qt5WebKit)
BuildRequires:	pkgconfig(tk)
BuildRequires:	pkgconfig(zlib)
BuildRequires:	python3-pillow-devel
BuildRequires:	python3-qt5-devel
BuildRequires:	python3-tkinter


%if 0%{?fedora} >= 23 || 0%{?rhel} > 7
Supplements:	%{name}-doc = %{version}-%{release}
%else
Requires:	%{name}-doc = %{version}-%{release}
%endif

%filter_provides_in %{_libdir}/%{name}/plugins
%filter_setup


%description
Scribus is an desktop open source page layout program with
the aim of producing commercial grade output in PDF and
Postscript, primarily, though not exclusively for Linux.

While the goals of the program are for ease of use and simple easy to
understand tools, Scribus offers support for professional publishing
features, such as CMYK color, easy PDF creation, Encapsulated Postscript
import/export and creation of color separations.

%package        devel
Summary:	Header files for Scribus
Requires:	%{name} = %{version}-%{release}

%description    devel
#Header files for Scribus.

%package        doc
Summary:	Documentation files for Scribus
Requires:       %{name} = %{version}-%{release}
%if 0%{?fedora} > 9
BuildArch:      noarch
Obsoletes:      %{name}-doc < 1.3.5-0.12.beta
%endif

%description    doc
%{summary}

%prep
%autosetup -n %{name}-%{build_branch}

# fix permissions
chmod a-x scribus/pageitem_latexframe.h

# drop shebang lines from python scripts
pathfix.py -pni "%{__python3} %{py3_shbang_opts}" \
	scribus/plugins/scriptplugin/{samples,scripts}/*.py

%build
mkdir build
pushd build
%cmake  -DWANT_CCACHE=YES \
	-DWANT_DISTROBUILD=YES \
	-DWANT_GRAPHICSMAGICK=1 \
	-DWANT_HUNSPELL=1 \
%ifarch x86_64 || aarch64
	-DWANT_LIB64=YES \
%endif
	-DWITH_BOOST=1 \
	-DWITH_PODOFO=1 ..

%make_build VERBOSE=1
popd

%install
pushd build
%make_install
popd

find %{buildroot} -type f -name "*.la" -exec rm -f {} ';'

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop
appstream-util validate-relax --nonet \
	%{buildroot}%{_metainfodir}/%{name}.appdata.xml


%files
%doc %{_defaultdocdir}/%{name}/AUTHORS
%doc %{_defaultdocdir}/%{name}/ChangeLog
%doc %{_defaultdocdir}/%{name}/COPYING
%doc %{_defaultdocdir}/%{name}/README
%{_bindir}/%{name}
%{_libdir}/%{name}/
%{_metainfodir}/%{name}.appdata.xml
%{_datadir}/applications/%{name}.desktop
%{_datadir}/mime/packages/%{name}.xml
%{_datadir}/icons/hicolor/16x16/apps/%{name}.png
%{_datadir}/icons/hicolor/32x32/apps/%{name}.png
%{_datadir}/icons/hicolor/128x128/apps/%{name}.png
%{_datadir}/icons/hicolor/256x256/apps/%{name}.png
%{_datadir}/icons/hicolor/512x512/apps/%{name}.png
%{_datadir}/icons/hicolor/1024x1024/apps/%{name}.png
%{_datadir}/%{name}/
%exclude %{_datadir}/%{name}/samples/*.py[co]
%exclude %{_datadir}/%{name}/scripts/*.py[co]
%{_mandir}/man1/*
%{_mandir}/pl/man1/*
%{_mandir}/de/man1/*

%files devel
%doc AUTHORS COPYING

%files doc
%dir %{_defaultdocdir}/%{name}
%lang(de) %{_defaultdocdir}/%{name}/de
%lang(en) %{_defaultdocdir}/%{name}/en
%lang(it) %{_defaultdocdir}/%{name}/it
	%lang(ru) %{_defaultdocdir}/%{name}/ru
%{_defaultdocdir}/%{name}/README*
%{_defaultdocdir}/%{name}/LINKS
%{_defaultdocdir}/%{name}/TRANSLATION

%changelog
* Mon Jan 13 2020 Luya Tshimbalanga <luya@fedoraproject.org>
- Automate build to reduce maintenance
- Use pkgconfig on many build requirements

* Wed Oct 30 2019 Luya Tshimbalanga <luya@fedoraproject.org> - 1.5.6-0-20191030git
- Snapshot svn 23306
- Drop no longer needed update database scribus
- Clean up spec file for adherance to Fedora packaging guideline

* Sun Aug 04 2019 Luya Tshimbalanga <luya@fedoraproject.org> - 1.5.6-0-20190804git
- Update to 1.5.6 snapshot svn 23099

* Mon Jul 15 2019 Luya Tshimbalanga <luya@fedoraproject.org> - 1.5.5-0-20190715git
- Snapshot svn 23086

* Mon Jul 01 2019 Luya Tshimbalanga <luya@fedoraproject.org> - 1.5.5-0-20190705git
- Snapshot svn 23067

