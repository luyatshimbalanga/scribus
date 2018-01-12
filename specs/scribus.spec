%global build_timestamp %{lua: print(os.date("%Y%m%d"))}

Name:		scribus
Version:	1.5.4
Release:	0.%{build_timestamp}git%{?dist}
Summary:	Open Source Page Layout
License:	GPLv2+
URL:		http://www.scribus.net/
# svn export svn://scribus.net/trunk/Scribus scribus
# tar --exclude-vcs -cJf scribus-1.5.0-20161204svn21568.tar.xz scribus
Source0:	https://github.com/%{name}project/%{name}/archive/master.tar.gz#/%{name}-%{version}-%{build_timestamp}git.tar.gz

BuildRequires:	boost-devel
BuildRequires:	cairo-devel
BuildRequires:	cmake
BuildRequires:	cups-devel
BuildRequires:	desktop-file-utils
BuildRequires:	fontconfig-devel
BuildRequires:	freetype-devel
BuildRequires:	ghostscript
BuildRequires:	gnutls-devel
BuildRequires:	GraphicsMagick-devel
BuildRequires:	GraphicsMagick-c++-devel
BuildRequires:	harfbuzz-devel
BuildRequires:	hyphen-devel
BuildRequires:	hunspell-devel
BuildRequires:	lcms2-devel
BuildRequires:	libappstream-glib
BuildRequires:	libcdr-devel
BuildRequires:	libfreehand-devel
BuildRequires:	libicu-devel
BuildRequires:	libjpeg-turbo-devel
BuildRequires:	libmspub-devel
BuildRequires:	libpagemaker-devel
BuildRequires:	libpng-devel
BuildRequires:	librevenge-devel
BuildRequires:	libtiff-devel
BuildRequires:	libvisio-devel
BuildRequires:	libwpd-devel
BuildRequires:	libwpg-devel
BuildRequires:	libxml2-devel
BuildRequires:	OpenSceneGraph-devel
BuildRequires:	openssl-devel
BuildRequires:	podofo-devel
BuildRequires:	poppler-cpp-devel
BuildRequires:	poppler-data-devel
BuildRequires:	poppler-devel
BuildRequires:	python-devel
%if 0%{?fedora} > 24
BuildRequires:	python2-pillow-devel
BuildRequires:	python3-pillow-devel
%else
BuildRequires:	python-pillow-devel
%endif
BuildRequires:	python-qt5-devel
Requires:	qt5-qtbase-gui
BuildRequires:	qt5-qtbase-devel
BuildRequires:	qt5-qtdeclarative-devel
BuildRequires:	qt5-qttools-devel
BuildRequires:	qt5-qtwebkit-devel
BuildRequires:	tk-devel
BuildRequires:	tkinter
BuildRequires:	zlib-devel


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
%autosetup -n %{name}-master

# fix permissions
chmod a-x scribus/pageitem_latexframe.h

# drop shebang lines from python scripts
for f in scribus/plugins/scriptplugin/{samples,scripts}/*.py
do
    sed '1{/#!\/usr\/bin\/env\|#!\/usr\/bin\/python/d}' $f > $f.new
    touch -r $f $f.new
    mv $f.new $f
done

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
	-DWANT_NORPATH=1 \
	-DWANT_VERSIONING=YES \
	-DWANT_CPP11=1 \
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
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}-%{version}.svn.desktop
appstream-util validate-relax --nonet \
	%{buildroot}/%{_datadir}/metainfo/%{name}-%{version}.svn.appdata.xml

%post
touch --no-create %{_datadir}/mime/packages &> /dev/null || :
touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :

%if 0%{?fedora} < 25
update-desktop-database &> /dev/null || :
%endif

%postun
if [ $1 -eq 0 ] ; then
	touch --no-create %{_datadir}/icons/hicolor &>/dev/null
	gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
touch --no-create %{_datadir}/mime/packages &> /dev/null || :
#

%if 0%{?fedora} < 25
	update-desktop-database &> /dev/null || :
%endif
	update-mime-database %{?fedora:-n} %{_datadir}/mime &> /dev/null || :
fi

%posttrans
%if 0%{?fedora} < 25
update-desktop-database &> /dev/null || :
%endif
update-mime-database %{?fedora:-n} %{_datadir}/mime &> /dev/null || :

# Update 
gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :

%files
%doc %{_defaultdocdir}/%{name}-%{version}.svn/AUTHORS
%doc %{_defaultdocdir}/%{name}-%{version}.svn/ChangeLog
# %%doc %%{_defaultdocdir}/%%{name}-%%{version}/ChangeLogSVN
%doc %{_defaultdocdir}/%{name}-%{version}.svn/COPYING
%doc %{_defaultdocdir}/%{name}-%{version}.svn/README
%{_bindir}/%{name}-%{version}.svn
%{_libdir}/%{name}-%{version}.svn/
%{_datadir}/metainfo/%{name}-%{version}.svn.appdata.xml
#%%{_datadir}/applications/%%{name}.desktop
%{_datadir}/applications/%{name}-%{version}.svn.desktop
%{_datadir}/mime/packages/%{name}-%{version}.svn.xml
#%%{_datadir}/pixmaps/*
%{_datadir}/icons/hicolor/16x16/apps/%{name}-%{version}.svn.png
#%%{_datadir}/icons/hicolor/24x24/apps/%%{name}-%%{version}.svn.png
%{_datadir}/icons/hicolor/32x32/apps/%{name}-%{version}.svn.png
#%%{_datadir}/icons/hicolor/64x64/apps/%%{name}-%%{version}.svn.png
%{_datadir}/icons/hicolor/128x128/apps/%{name}-%{version}.svn.png
%{_datadir}/icons/hicolor/256x256/apps/%{name}-%{version}.svn.png
%{_datadir}/icons/hicolor/512x512/apps/%{name}-%{version}.svn.png
%{_datadir}/icons/hicolor/1024x1024/apps/%{name}-%{version}.svn.png
%{_datadir}/%{name}-%{version}.svn/
%exclude %{_datadir}/%{name}-%{version}.svn/samples/*.py[co]
%exclude %{_datadir}/%{name}-%{version}.svn/scripts/*.py[co]
%{_mandir}/man1/*
%{_mandir}/pl/man1/*
%{_mandir}/de/man1/*

%files devel
%doc AUTHORS COPYING

%files doc
%dir %{_defaultdocdir}/%{name}-%{version}.svn
%lang(de) %{_defaultdocdir}/%{name}-%{version}.svn/de
%lang(en) %{_defaultdocdir}/%{name}-%{version}.svn/en
%lang(it) %{_defaultdocdir}/%{name}-%{version}.svn/it
%{_defaultdocdir}/%{name}-%{version}.svn/README*
%{_defaultdocdir}/%{name}-%{version}.svn/LINKS
%{_defaultdocdir}/%{name}-%{version}.svn/TRANSLATION

%changelog
* Tue Jan 09 2018 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0-20180109git
- Snapshot svn 22338

* Fri Jan 05 2018 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0-20180105git
- Snapshot svn 22331

* Sat Dec 30 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0-20171230git
- Snapshot svn 22309

* Fri Dec 29 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0-20171228git
- Snapshot svn 22292

* Tue Dec 19 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0-20171219git
- Snapshot svn 22261

* Sat Dec 16 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0-20171216git
- Snapshot svn 22252

* Tue Dec 12 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.20171212git
- Snapshot 20171212

* Wed Dec 6 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.20171206git
- Snapshot 20171206

* Thu Nov 30 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.29.20171129svn22228
- Update to svn 22228

* Fri Nov 24 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.28.20171123svn22224
- Update to svn 22224

* Thu Nov 23 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.27.20171122svn22223
- Update to svn 22223

* Fri Nov 10 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.26.20171110svn22219
- Update to svn 22219

* Fri Nov 03 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.25.20171102svn22214
- Update to svn 22214

* Fri Oct 27 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.24.20171027svn22205
- Update to svn 22205

* Sat Oct 21 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.23.20171021svn22198
- Update to svn 22198

* Sat Oct 14 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.22.20171013svn22193
- Update to svn 22193

* Thu Oct 12 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.21.20171011svn22191
- Update to svn 22191

* Sat Oct 07 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.20.20171006svn22185
- Update to svn 22185

* Thu Oct 05 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.19.20171005svn22182
- Update to svn 22182

* Mon Oct 02 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.18.20171001svn22175
- Update to 22175

* Tue Sep 26 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.17.20170926svn22167
- Update to svn 22167

* Fri Sep 22 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.16.20170922svn22163
- Update to svn 22163

* Sun Sep 17 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.15.20170917svn22157
- Update to svn 22157

* Sat Sep 02 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.14.20170901svn22151
- Update to svn 22151

* Thu Aug 24 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.13.20170824svn22141
- Update to svn 22141

* Tue Aug 15 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.12.20170814svn22134
- Update to svn 22134

* Tue Jul 25 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.11.20170724svn22121
- rebuilt

* Fri Jul 21 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.10.20170720svn22117
- rebuilt

* Sun Jul 16 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.9.20170715svn22112
- Update to svn 22112

* Sat Jul 08 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.8.20170708svn22104
- Update to svn 22104

* Thu Jun 29 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.7.20170628svn22092
- Update to svn 22092

* Sat Jun 24 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.6.20170624svn22083
- Update to svn 22083

* Tue Jun 20 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.5.20170620svn22074
- Update to svn 22074

* Wed Jun 14 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.4.20170613svn22065
- Update to svn 22065

* Sun Jun 11 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.3.20170610svn22060
- Update to svn 22060

* Fri Jun 09 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.2.20170609svn22059
- Update to svn 22059

* Mon Jun 05 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.4-0.1.20170605svn22056
- Update to svn 22056

* Thu Jun 01 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-1
- 1.5.3 release

* Mon May 29 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.74.20170528svn22048
- Update to svn 22048

* Thu May 25 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.73.20170523svn22044
- Update to svn 22044

* Tue May 23 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.72.20170523svn22040
- Update to svn 22040

* Mon May 22 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.71.20170521svn22038
- Update to svn 22038

* Thu May 18 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.70.20170517svn22029
- Update to svn 22029

* Mon May 15 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.69.20170510svn22018
- Update to svn 22024

* Thu May 11 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.68.20170510svn22018
- Update to svn 22018

* Sun May 07 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.67.20170506svn22015
- Update to svn 22015

* Mon May 01 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.66.20170501svn22011
- Update to svn 22011

* Sat Apr 29 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.65.20170428svn22006
- Update to svn 22006

* Wed Apr 26 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.64.20170425svn22004
- Update to svn 22004

* Mon Apr 24 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.63.20170423svn22002
- Update to svn 22002

* Fri Apr 21 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.62.20170421svn21958
- Update to svn 21958

* Mon Apr 17 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.61.20170417svn21927
- Update to svn 21927

* Thu Apr 06 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.60.20170405svn21869
- Update to svn 21869

* Sun Apr 02 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.59.20170401svn21862
- Update to svn 21862

* Thu Mar 30 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.58.20170330svn21856
- Update to svn 21856

* Sat Mar 25 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.57.20170325svn21853
- Update to svn 21853

* Tue Mar 21 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.56.20170321svn21849
- Update to svn 21849

* Mon Mar 20 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.55.20170320svn21844
- Update to svn 21844

* Sun Mar 19 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.54.20170319svn21840
- Update to svn 21840

* Sat Mar 18 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.53.20170317svn21836
- Update to svn 21836

* Tue Mar 14 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.52.20170314svn21824
- Update to svn 21824

* Mon Mar 13 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.51.20170313svn21821
- Update to svn 21821

* Sun Mar 12 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.50.20170311svn21817
- Update to svn 21817

* Fri Mar 10 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.49.20170309svn21814
- Update to svn 21814

* Tue Mar 07 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.48.20170307svn21813
- Update to svn 21813

* Sat Mar 04 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.47.20170304svn21809
- Update to svn 21809

* Wed Mar 01 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.46.20170301svn21800
- Update to svn 21800

* Fri Feb 24 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.45.20170224svn21794
- update to svn 21794

* Tue Feb 21 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.44.20170220svn21790
- Update to svn 21790

* Fri Feb 17 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.43.20170217svn21783
- update to svn 21783

* Tue Feb 14 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.42.20170214svn21780
- update to svn 21780
- shorten changelog

* Tue Feb 07 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.40.20170207svn21756
- update to svn 21756

* Mon Jan 30 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.39.20170129svn21748
- update to svn 21748

* Tue Jan 24 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.38.20170124svn21742
- update to svn 21742

* Fri Jan 20 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.37.20170119svn21736
- update to svn 21736

* Sat Jan 14 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.36.20170113svn21729
- update to svn 21729

* Thu Jan 12 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.35.20170112svn21728
- update to svn 21728

* Mon Jan 09 2017 Luya Tshimbalanga <luya_tfz@thefinalzone.net> - 1.5.3-0.34.20170109svn21722
- update to svn 21722

* Thu Jan 05 2017 Luya Tshimbalanga <luya@fedoraproject.org> - 1.5.3-0.33.20170105svn21700
- update to svn 21700
