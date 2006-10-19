%define svn 697
#%define rel 5

%if %defined svn
%define release %mkrel 0.svn%svn.%rel
%else
%define release %mkrel %rel
%endif

%define name emerald

%define lib_name_orig %mklibname %{name}
%define lib_major 0
%define lib_name %lib_name_orig%lib_major

Name: %name
Version: 0.1.1
Release: %release
Summary: Window decorator for beryl
Group: System/X11
URL: http://www.beryl-project.org/
# NB Tarballs generated from http://svn.beryl-project.org/tags/release-%{version}/%{name}/
%if %defined svn
Source: %{name}-%{version}-%{svn}.tar.bz2
%else
Source: %{name}-%{version}.tar.bz2
%endif
License: GPL
BuildRoot: %{_tmppath}/%{name}-root
BuildRequires: beryl-core-devel
BuildRequires: png-devel
BuildRequires: X11-devel
BuildRequires: libgtk+2.0-devel
BuildRequires: pkgconfig(libwnck-1.0)
BuildRequires: pkgconfig(libstartup-notification-1.0)
BuildRequires: pkgconfig(dbus-glib-1)
BuildRequires: apr-devel
BuildRequires: apr-util-devel
BuildRequires: subversion-devel
BuildRequires: neon-devel
BuildRequires: intltool
BuildRequires: desktop-file-utils
Requires(post): desktop-file-utils
Requires(postun): desktop-file-utils

Requires: beryl-core
Requires: emerald-themes

# cgwd was renamed to emerald
Provides: cgwd
Obsoletes: cgwd

%description
Themable window decorator for the Beryl window manager/compositor

%package -n %lib_name
Summary: Headers files for %{name}
Group: System/X11
Requires: %{name} = %{version}
Provides: %lib_name = %version

%description -n %lib_name
Headers files for %{name}

%post -n %lib_name -p /sbin/ldconfig

%postun -n %lib_name -p /sbin/ldconfig

%package -n %lib_name-devel
Summary:Development files from %{name}
Group: Development/Other
Requires: %lib_name = %{version}
Provides: lib%{name}-devel = %{version}
Provides: %{name}-devel = %{version}
Obsoletes: %{name}-devel
Obsoletes: cgwd-devel

%description -n %lib_name-devel
Headers files for %{name}

%post -n %lib_name-devel -p /sbin/ldconfig


%prep
%setup -q -n %{name}

sh autogen.sh -V

%configure2_5x \
                --x-includes=%{_includedir}\
                --x-libraries=%{_libdir} \
                --disable-mime-update \
                --with-apr-config=apr-1-config \
                --with-apu-config=apu-1-config \
                --with-svn-lib=%_libdir

%make


%install
rm -rf %buildroot
%makeinstall_std

sed -i 's/Exec=emerald-theme-manager -i//' $RPM_BUILD_ROOT%{_datadir}/applications/emerald-theme-manager.desktop

desktop-file-install \
  --vendor="" \
  --remove-category="Settings" \
  --add-category="X-MandrivaLinux-System-Configuration-Other" \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications \
  $RPM_BUILD_ROOT%{_datadir}/applications/*.desktop

%clean
rm -rf %buildroot

%post
%update_menus
%{update_desktop_database}

%postun
%clean_menus
%{clean_desktop_database}

%files
%defattr(-,root,root)
%doc AUTHORS  ChangeLog README COPYING 
%{_bindir}/emerald
%{_bindir}/emerald-theme-manager
%{_datadir}/applications/emerald-theme-manager.desktop
%{_datadir}/mime-info/emerald.mime
%{_datadir}/mime/packages/emerald.xml
%{_datadir}/pixmaps/emerald-theme-manager-icon.png
%{_datadir}/icons/hicolor/48x48/mimetypes/application-x-emerald-theme.png
%{_datadir}/%{name}/*
# FIXME: create i18n packages
%{_datadir}/locale/*/LC_MESSAGES/%{name}.mo
%{_mandir}/man1/emerald-theme-manager.1.bz2
%{_mandir}/man1/emerald.1.bz2


%files -n %lib_name-devel
%defattr(-,root,root)
%{_libdir}/pkgconfig/emeraldengine.pc
%{_includedir}/emerald/*
%{_libdir}/libemeraldengine.*
%{_libdir}/emerald/engines/*

%files -n %lib_name
%defattr(-,root,root)
%{_libdir}/libemeraldengine.so.*
%{_libdir}/emerald/engines/*


