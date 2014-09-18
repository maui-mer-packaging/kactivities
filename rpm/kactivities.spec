# 
# Do NOT Edit the Auto-generated Part!
# Generated by: spectacle version 0.27
# 

Name:       kactivities

# >> macros
# << macros

# >> bcond_with
# << bcond_with

# >> bcond_without
# << bcond_without

Summary:    A KDE Frameworks 5 Tier 3 runtime and library to organize activities
Version:    5.2.0
Release:    1
Group:      System/Base
License:    GPLv2+
URL:        http://www.kde.org
Source0:    %{name}-%{version}.tar.xz
Source100:  kactivities.yaml
Source101:  kactivities-rpmlintrc
Requires:   kf5-filesystem
Requires:   kactivities-libs%{?_isa} = %{version}-%{release}
Requires:   kactivities-runtime%{?_isa} = %{version}-%{release}
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
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  kf5-rpm-macros
BuildRequires:  extra-cmake-modules
BuildRequires:  qt5-tools
BuildRequires:  kconfig-devel
BuildRequires:  kcoreaddons-devel
BuildRequires:  ki18n-devel
BuildRequires:  kservice-devel
BuildRequires:  kwindowsystem-devel
BuildRequires:  kdbusaddons-devel
BuildRequires:  kconfig-devel
BuildRequires:  kjs-devel
BuildRequires:  kio-devel
BuildRequires:  kdeclarative-devel
BuildRequires:  kcmutils-devel
BuildRequires:  boost-devel

%description
A KDE Frameworks 5 Tier 3 API for using and interacting with Activities as a
consumer, application adding information to them or as an activity manager.


%package libs
Summary:    Libraries from KActivities framework
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   kf5-filesystem

%description libs
libs

%package libs-devel
Summary:    Developer files for %{name}-libs
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   kconfig-devel
Requires:   kcoreaddons-devel
Requires:   ki18n-devel
Requires:   kservice-devel
Requires:   kwindowsystem-devel
Requires:   kdbusaddons-devel
Requires:   kconfig-devel
Requires:   kjs-devel
Requires:   kio-devel
Requires:   kdeclarative-devel

%description libs-devel
The %{name}-devel package contains the files necessary to develop applications
that use %{name}.


%package runtime
Summary:    Runtime for KActivities framework
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}

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
%{_kf5_cmakedir}/KF5Activities/
%{_kf5_includedir}/KActivities/
%{_kf5_includedir}/kactivities_version.h
%{_kf5_libdir}/pkgconfig/libKActivities.pc
%{_kf5_mkspecsdir}/qt_KActivities.pri
# >> files libs-devel
# << files libs-devel

%files runtime
%defattr(-,root,root,-)
%{_kf5_bindir}/kactivitymanagerd
%{_kf5_servicesdir}/kactivitymanagerd.desktop
%{_kf5_servicesdir}/kactivitymanagerd-plugin-*.desktop
%{_kf5_servicesdir}/activities.protocol
%{_kf5_servicetypesdir}/kactivitymanagerd-plugin.desktop
%{_kf5_plugindir}/*
%{_kf5_servicesdir}/kcm_activities.desktop
%{_kf5_datadir}/kactivitymanagerd
# >> files runtime
# << files runtime