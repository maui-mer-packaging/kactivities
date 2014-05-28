# 
# Do NOT Edit the Auto-generated Part!
# Generated by: spectacle version 0.22
# 
# >> macros
# << macros

Name:       kf5-kactivities
Summary:    A KDE Frameworks 5 Tier 3 runtime and library to organize the user work in separate activitie
Version:    4.99.0
Release:    1
Group:      System/Base
License:    GPLv2+
URL:        http://www.kde.org
Source0:    %{name}-%{version}.tar.xz
Source100:  kf5-kactivities.yaml
Source101:  kf5-kactivities-rpmlintrc
Requires:   kf5-filesystem
Requires:   kf5-kactivities-libs%{?_isa} = %{version}-%{release}
Requires:   kf5-kactivities-runtime%{?_isa} = %{version}-%{release}
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Xml)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Widgets)
BuildRequires:  pkgconfig(Qt5Concurrent)
BuildRequires:  pkgconfig(Qt5Test)
BuildRequires:  pkgconfig(Qt5Sql)
BuildRequires:  kf5-rpm-macros
BuildRequires:  extra-cmake-modules
BuildRequires:  qt5-tools
BuildRequires:  kf5-umbrella
BuildRequires:  kf5-kconfig-devel
BuildRequires:  kf5-kcoreaddons-devel
BuildRequires:  kf5-ki18n-devel
BuildRequires:  kf5-kservice-devel
BuildRequires:  kf5-kwindowsystem-devel
BuildRequires:  kf5-kdbusaddons-devel
BuildRequires:  kf5-kconfig-devel
BuildRequires:  kf5-kjs-devel


%description
A KDE Frameworks 5 Tier 3 runtime and library to organize the user work in separate activitie



%package libs
Summary:    Libraries fro KActivities framework
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   kf5-filesystem

%description libs
libs

%package libs-devel
Summary:    Developer files for %{name}-libs
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Conflicts:   kactivities-devel

%description libs-devel
The %{name}-devel package contains the files necessary to develop applications
that use %{name}.


%package runtime
Summary:    Runtime for KActivities framework
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}
Provides:   kactivities = 0:%{version}-%{release}
Conflicts:   kactivities < 0:4.90.0

%description runtime
The runtime module is a drop-in replacement for KActivities runtime module from
KDE 4 and can be safely used with KDE 4.



%prep
%setup -q -n %{name}-%{version}/upstream

# >> setup
# << setup

%build
# >> build pre
%kf5_make
# << build pre



# >> build post
# << build post
%install
rm -rf %{buildroot}
# >> install pre
%kf5_make_install
# << install pre

# >> install post
# << install post















%files
%defattr(-,root,root,-)
%doc README README.md README.packagers README.developers MAINTAINER
# >> files
# << files


%files libs
%defattr(-,root,root,-)
%{_kf5_libdir}/libKF5Activities.so.*
%{_kf5_qmldir}/org/kde/activities/
# >> files libs
# << files libs

%files libs-devel
%defattr(-,root,root,-)
%{_kf5_libdir}/libKF5Activities.so
%{_kf5_libdir}/cmake/KF5Activities/
%{_kf5_includedir}/KActivities/
%{_kf5_includedir}/kactivities_version.h
%{_kf5_libdir}/pkgconfig/libKActivities.pc
%{_datadir}/qt5/mkspecs/modules/qt_KActivities.pri
# >> files libs-devel
# << files libs-devel

%files runtime
%defattr(-,root,root,-)
%{_kf5_bindir}/kactivitymanagerd
%{_kf5_datadir}/kservices5/kactivitymanagerd.desktop
%{_kf5_datadir}/kservicetypes5/kactivitymanagerd-plugin.desktop
%{_kf5_qtplugindir}/kf5/*
# >> files runtime
# << files runtime

