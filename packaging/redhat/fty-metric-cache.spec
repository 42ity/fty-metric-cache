#
#    fty-metric-cache - Knows current values of any METRIC in the system
#
#    Copyright (C) 2014 - 2017 Eaton
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

# To build with draft APIs, use "--with drafts" in rpmbuild for local builds or add
#   Macros:
#   %_with_drafts 1
# at the BOTTOM of the OBS prjconf
%bcond_with drafts
%if %{with drafts}
%define DRAFTS yes
%else
%define DRAFTS no
%endif
%define SYSTEMD_UNIT_DIR %(pkg-config --variable=systemdsystemunitdir systemd)
Name:           fty-metric-cache
Version:        1.0.0
Release:        1
Summary:        knows current values of any metric in the system
License:        GPL-2.0+
URL:            https://42ity.org
Source0:        %{name}-%{version}.tar.gz
Group:          System/Libraries
# Note: ghostscript is required by graphviz which is required by
#       asciidoc. On Fedora 24 the ghostscript dependencies cannot
#       be resolved automatically. Thus add working dependency here!
BuildRequires:  ghostscript
BuildRequires:  asciidoc
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:  pkgconfig
BuildRequires:  systemd-devel
BuildRequires:  systemd
%{?systemd_requires}
BuildRequires:  xmlto
BuildRequires:  libsodium-devel
BuildRequires:  zeromq-devel
BuildRequires:  czmq-devel
BuildRequires:  malamute-devel
BuildRequires:  fty-proto-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
fty-metric-cache knows current values of any metric in the system.

%package -n libfty_metric_cache1
Group:          System/Libraries
Summary:        knows current values of any metric in the system shared library

%description -n libfty_metric_cache1
This package contains shared library for fty-metric-cache: knows current values of any metric in the system

%post -n libfty_metric_cache1 -p /sbin/ldconfig
%postun -n libfty_metric_cache1 -p /sbin/ldconfig

%files -n libfty_metric_cache1
%defattr(-,root,root)
%{_libdir}/libfty_metric_cache.so.*

%package devel
Summary:        knows current values of any metric in the system
Group:          System/Libraries
Requires:       libfty_metric_cache1 = %{version}
Requires:       libsodium-devel
Requires:       zeromq-devel
Requires:       czmq-devel
Requires:       malamute-devel
Requires:       fty-proto-devel

%description devel
knows current values of any metric in the system development tools
This package contains development files for fty-metric-cache: knows current values of any metric in the system

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libfty_metric_cache.so
%{_libdir}/pkgconfig/libfty_metric_cache.pc
%{_mandir}/man3/*
%{_mandir}/man7/*

%prep

%setup -q

%build
sh autogen.sh
%{configure} --enable-drafts=%{DRAFTS} --with-systemd-units
make %{_smp_mflags}

%install
make install DESTDIR=%{buildroot} %{?_smp_mflags}
mkdir -p %{buildroot}/%{_sysconfdir}/fty-metric-cache

# remove static libraries
find %{buildroot} -name '*.a' | xargs rm -f
find %{buildroot} -name '*.la' | xargs rm -f

%files
%defattr(-,root,root)
%{_bindir}/fty-metric-cache
%{_mandir}/man1/fty-metric-cache*
%{_bindir}/fty-metric-cache-cli
%{_mandir}/man1/fty-metric-cache-cli*
%{SYSTEMD_UNIT_DIR}/fty-metric-cache.service
%dir %{_sysconfdir}/fty-metric-cache
%if 0%{?suse_version} > 1315
%post
%systemd_post fty-metric-cache.service
%preun
%systemd_preun fty-metric-cache.service
%postun
%systemd_postun_with_restart fty-metric-cache.service
%endif

%changelog
