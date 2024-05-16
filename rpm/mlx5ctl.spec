Summary: ConnectX device control utility
Name: mlx5ctl
Version: %{APP_VERSION}
Release: 1
License: BSD-3-Clause
Group: Development/Tools
URL: https://github.com/saeedtx/mlx5ctl
Source0: mlx5ctl-%{APP_VERSION}.tar.gz

BuildRequires: binutils
BuildRequires: cmake >= 3.18

# Ninja was introduced in FC23
BuildRequires: ninja-build
%define CMAKE_FLAGS -GNinja
%if 0%{?fedora} >= 33 || 0%{?rhel} >= 9
%define make_jobs ninja-build -C %{_vpath_builddir} -v %{?_smp_mflags}
%define cmake_install DESTDIR=%{buildroot} ninja-build -C %{_vpath_builddir} install
%else
%define make_jobs ninja-build -v %{?_smp_mflags}
%define cmake_install DESTDIR=%{buildroot} ninja-build install
%endif

%description
Userspace Linux Debug Utilities for mlx5 ConnectX Devices

%prep
%setup -q

%build

# New RPM defines _rundir, usually as /run
%if 0%{?_rundir:1}
%else
%define _rundir /var/run
%endif

# Pass all of the rpm paths directly to GNUInstallDirs and our other defines.
%cmake %{CMAKE_FLAGS} \
         -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_BINDIR:PATH=%{_bindir} \
         -DCMAKE_INSTALL_SBINDIR:PATH=%{_sbindir} \
         -DCMAKE_INSTALL_LIBDIR:PATH=%{_lib} \
         -DCMAKE_INSTALL_LIBEXECDIR:PATH=%{_libexecdir} \
         -DCMAKE_INSTALL_LOCALSTATEDIR:PATH=%{_localstatedir} \
         -DCMAKE_INSTALL_SHAREDSTATEDIR:PATH=%{_sharedstatedir} \
         -DCMAKE_INSTALL_INCLUDEDIR:PATH=include \
         -DCMAKE_INSTALL_INFODIR:PATH=%{_infodir} \
         -DCMAKE_INSTALL_MANDIR:PATH=%{_mandir} \
         -DCMAKE_INSTALL_SYSCONFDIR:PATH=%{_sysconfdir} \
         -DCMAKE_INSTALL_SYSTEMD_SERVICEDIR:PATH=%{_unitdir} \
         -DCMAKE_INSTALL_INITDDIR:PATH=%{_initrddir} \
         -DCMAKE_INSTALL_RUNDIR:PATH=%{_rundir} \
         -DCMAKE_INSTALL_DOCDIR:PATH=%{_docdir}/%{name} \
         -DCMAKE_INSTALL_UDEV_RULESDIR:PATH=%{_udevrulesdir} \
         -DCMAKE_INSTALL_PERLDIR:PATH=%{perl_vendorlib}
%make_jobs

%install
%cmake_install

%files
%{_bindir}/mlx5ctl
#%{_mandir}/man1/mlx5ctl.1.gz  # Example man page if available
