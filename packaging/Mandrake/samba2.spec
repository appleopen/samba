Summary: Samba SMB server.
Name: samba
Version: 2.2.3a
Release: 20020206
License: GNU GPL version 2
Group: System/Servers
Packager: John H Terpstra [samba-team] <jht@samba.org>

Source: ftp://samba.org/pub/samba/samba-%{version}.tar.bz2
Source1: samba.log
Source2: mount.smb
Source3: samba.xinetd
Source4: swat_48.xpm.bz2
Source5: swat_32.xpm.bz2
Source6: swat_16.xpm.bz2

Patch: smbw.patch.bz2
Patch1: samba-2.2.0-gawk.patch.bz2
Patch2: samba-2.2.0-buildroot.patch.bz2
Patch3: smbmount-sbin.patch.bz2

Requires: samba-common = %{version} pam >= 0.72 kernel >= 2.2.1 glibc >= 2.1.2
Prereq: xinetd chkconfig fileutils sed /bin/grep 
Prereq: /sbin/chkconfig /bin/mktemp /usr/bin/killall
BuildRequires: libcups-devel pam-devel
BuildRoot: %{_tmppath}/%{name}-root
Prefix: /usr

%description
Samba provides an SMB server which can be used to provide
network services to SMB (sometimes called "Lan Manager")
clients, including various versions of MS Windows, OS/2,
and other Linux machines. Samba also provides some SMB
clients, which complement the built-in SMB filesystem
in Linux. Samba uses NetBIOS over TCP/IP (NetBT) protocols
and does NOT need NetBEUI (Microsoft Raw NetBIOS frame)
protocol.

Samba-2.2 features working NT Domain Control capability and
includes the SWAT (Samba Web Administration Tool) that
allows samba's smb.conf file to be remotely managed using your
favourite web browser. For the time being this is being
enabled on TCP port 901 via xinetd.

Users are advised to use Samba-2.2 as a Windows NT4
Domain Controller only on networks that do NOT have a Windows
NT Domain Controller. This release does NOT as yet have
Backup Domain control ability.

Please refer to the WHATSNEW.txt document for fixup information.
This binary release includes encrypted password support.

Please read the smb.conf file and ENCRYPTION.txt in the
docs directory for implementation details.

%package client
Summary: Samba (SMB) client programs.
Group: Networking/Other
Requires: samba-common = %{version}
Obsoletes: smbfs

%description client
Samba-client provides some SMB clients, which complement the built-in
SMB filesystem in Linux. These allow the accessing of SMB shares, and
printing to SMB printers.

%package common
Summary: Files used by both Samba servers and clients.
Group: System/Servers

%description common
Samba-common provides files necessary for both the server and client
packages of Samba.

%package doc
Summary: Documentation for Samba servers and clients.
Group: System/Servers
Requires: samba-common = %{version}

%description doc
Samba-doc provides documentation files for both the server and client
packages of Samba.

%prep
%setup -q
%patch -p1 -b .smbw
%patch1 -p1 -b .gawk
%patch2 -p1 -b .buildroot
%patch3 -p1

%build
cd source
autoconf

NUMCPU=`grep processor /proc/cpuinfo | wc -l`

CFLAGS="$RPM_OPT_FLAGS"
%configure      --prefix=%{prefix} \
                --libdir=/etc/samba \
	        --localstatedir=/var \
                --sysconfdir=/etc/samba \
		--with-acl-support	\
                --with-automount \
                --with-codepagedir=/var/lib/samba/codepages \
                --with-configdir=/etc/samba \
	        --with-fhs \
                --with-mmap \
                --with-netatalk \
	        --with-pam \
                --with-pam_smbpass \
	        --with-privatedir=/etc/samba \
	        --with-quotas \
	        --with-sambabook=%{prefix}/share/swat/using_samba \
                --with-smbmount \
                --with-smbwrapper \
                --with-swatdir=%{prefix}/share/swat \
                --with-syslog \
                --with-utmp 

make -j${NUMCPU} proto
make -j${NUMCPU} CFLAGS="$RPM_OPT_FLAGS -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE" all smbfilter smbwrapper smbcacls pam_smbpass nsswitch nsswitch/libnss_wins.so debug2html

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/sbin
mkdir -p $RPM_BUILD_ROOT/etc/samba
mkdir -p $RPM_BUILD_ROOT/etc/{logrotate.d,pam.d,xinetd.d}
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
mkdir -p $RPM_BUILD_ROOT/%{prefix}/{bin,sbin}
mkdir -p $RPM_BUILD_ROOT/%{prefix}/share/swat/{images,help,include,using_samba}
mkdir -p $RPM_BUILD_ROOT/%{prefix}/share/swat/using_samba/{figs,gifs}
mkdir -p $RPM_BUILD_ROOT/%{_mandir}/{man1,man5,man7,man8}
mkdir -p $RPM_BUILD_ROOT/var/cache/samba
mkdir -p $RPM_BUILD_ROOT/var/log/samba
mkdir -p $RPM_BUILD_ROOT/var/spool/samba
mkdir -p $RPM_BUILD_ROOT/var/lib/samba/{netlogon,profiles,printers}
mkdir -p $RPM_BUILD_ROOT/var/lib/samba/codepages/src
mkdir -p $RPM_BUILD_ROOT/lib/security
mkdir -p $RPM_BUILD_ROOT%prefix/lib

# Install standard binary files

for i in nmblookup smbclient smbpasswd smbstatus testparm testprns \
    make_smbcodepage make_unicodemap make_printerdef rpcclient smbspool \
    smbcacls smbclient smbmount smbumount smbsh
do
  install -m755 source/bin/$i $RPM_BUILD_ROOT/%{prefix}/bin
done

install -m 755 source/bin/smbwrapper.so $RPM_BUILD_ROOT%prefix/lib/smbwrapper.so
install -m 755 source/bin/pam_smbpass.so $RPM_BUILD_ROOT/lib/security/pam_smbpass.so

#for i in addtosmbpass mksmbpasswd.sh smbtar convert_smbpasswd

for i in mksmbpasswd.sh smbtar convert_smbpasswd
do
  install -m755 source/script/$i $RPM_BUILD_ROOT/%{prefix}/bin
done

# Install secure binary files

for i in smbd nmbd swat smbfilter debug2html smbmnt smbcontrol
do
  install -m755 source/bin/$i $RPM_BUILD_ROOT/%{prefix}/sbin
done

# Install level 1,5,7,8 man pages

for mpl in 1 5 7 8;do
  mp=$(ls docs/manpages/*.$mpl)
  for i in $mp;do
  install -m644 $i $RPM_BUILD_ROOT/%{_mandir}/man$mpl
  done
done

# Install codepage source files

for i in 437 737 775 850 852 861 866 932 936 949 950 1251
do
  install -m644 source/codepages/codepage_def.$i $RPM_BUILD_ROOT/var/lib/samba/codepages/src
done

for i in 437 737 850 852 861 866 932 936 949 950 ISO8859-1 ISO8859-2 ISO8859-5 ISO8859-7 KOI8-R
do
  install -m644 source/codepages/CP$i.TXT $RPM_BUILD_ROOT/var/lib/samba/codepages/src
done

# Build codepage load files
for i in 437 737 775 850 852 861 866 932 936 949 950 1251; do
        $RPM_BUILD_ROOT/%{prefix}/bin/make_smbcodepage c $i $RPM_BUILD_ROOT/var/lib/samba/codepages/src/codepage_def.$i $RPM_BUILD_ROOT/var/lib/samba/codepages/codepage.$i
done

# Build unicode load files
for i in 437 737 850 852 861 866 932 936 949 950 ISO8859-1 ISO8859-2 ISO8859-5 ISO8859-7 KOI8-R; do
         $RPM_BUILD_ROOT/%{prefix}/bin/make_unicodemap $i $RPM_BUILD_ROOT/var/lib/samba/codepages/src/CP$i.TXT $RPM_BUILD_ROOT/var/lib/samba/codepages/unicode_map.$i
done
rm -rf $RPM_BUILD_ROOT/var/lib/samba/codepages/src

# Install the nsswitch library extension file
install -m755 source/nsswitch/libnss_wins.so $RPM_BUILD_ROOT/lib
# Make link for wins resolver
( cd $RPM_BUILD_ROOT/lib; ln -s libnss_wins.so libnss_wins.so.2; )

# Install SWAT helper files
        for i in swat/help/*.html docs/htmldocs/*.html; do
                install -m644 $i $RPM_BUILD_ROOT/usr/share/swat/help
        done

        for i in swat/images/*.gif; do
                install -m644 $i $RPM_BUILD_ROOT/usr/share/swat/images
        done

        for i in swat/include/*.html; do
                install -m644 $i $RPM_BUILD_ROOT/usr/share/swat/include
        done

# Install the O'Reilly "Using Samba" book

        for i in docs/htmldocs/using_samba/*.html; do
                install -m644 $i $RPM_BUILD_ROOT/usr/share/swat/using_samba
        done

        for i in docs/htmldocs/using_samba/gifs/*.gif; do
                install -m644 $i $RPM_BUILD_ROOT/usr/share/swat/using_samba/gifs
        done

        for i in docs/htmldocs/using_samba/figs/*.gif; do
                install -m644 $i $RPM_BUILD_ROOT/usr/share/swat/using_samba/figs
        done

# Install other stuff

        install -m644 packaging/Mandrake/smb.conf $RPM_BUILD_ROOT/etc/samba/smb.conf
        install -m644 packaging/Mandrake/smbusers $RPM_BUILD_ROOT/etc/samba/smbusers
        install -m755 packaging/Mandrake/smbprint $RPM_BUILD_ROOT/usr/bin
        #install -m755 packaging/RedHat/smbadduser $RPM_BUILD_ROOT/usr/bin
        install -m755 packaging/Mandrake/findsmb $RPM_BUILD_ROOT/usr/bin
        install -m755 packaging/Mandrake/smb.init $RPM_BUILD_ROOT/etc/rc.d/init.d/smb
        install -m755 packaging/Mandrake/smb.init $RPM_BUILD_ROOT/usr/sbin/samba
        install -m644 packaging/Mandrake/samba.pamd $RPM_BUILD_ROOT/etc/pam.d/samba
        install -m644 $RPM_SOURCE_DIR/samba.log $RPM_BUILD_ROOT/etc/logrotate.d/samba

# Link smbmount to /sbin/mount.smb and /sbin/mount.smbfs

        ln -s /%{prefix}/bin/smbmount $RPM_BUILD_ROOT/sbin/mount.smb
        ln -s /%{prefix}/bin/smbmount $RPM_BUILD_ROOT/sbin/mount.smbfs
        echo 127.0.0.1 localhost > $RPM_BUILD_ROOT/etc/samba/lmhosts

# Link smbspool to CUPS (does not require installed CUPS)

        mkdir -p $RPM_BUILD_ROOT/usr/lib/cups/backend
        ln -s /usr/bin/smbspool $RPM_BUILD_ROOT/usr/lib/cups/backend/smb

# xinetd support

        mkdir -p $RPM_BUILD_ROOT/etc/xinetd.d
        install -m644 %{SOURCE3} $RPM_BUILD_ROOT/etc/xinetd.d/swat

# menu support

mkdir -p $RPM_BUILD_ROOT%{_menudir}
cat > $RPM_BUILD_ROOT%{_menudir}/%{name} << EOF
?package(%{name}):command="gnome-moz-remote http://localhost:901/" needs="gnome" \
icon="swat.xpm" section="Configuration/Networking" title="Samba Configuration" \
longtitle="The Swat Samba Administration Tool"
?package(%{name}):command="sh -c '\$BROWSER http://localhost:901/'" needs="x11" \
icon="swat.xpm" section="Configuration/Networking" title="Samba Configuration" \
longtitle="The Swat Samba Administration Tool"
EOF

mkdir -p $RPM_BUILD_ROOT%{_liconsdir} $RPM_BUILD_ROOT%{_iconsdir} $RPM_BUILD_ROOT%{_miconsdir}

bzcat %{SOURCE4} > $RPM_BUILD_ROOT%{_liconsdir}/swat.xpm
bzcat %{SOURCE5} > $RPM_BUILD_ROOT%{_iconsdir}/swat.xpm
bzcat %{SOURCE6} > $RPM_BUILD_ROOT%{_miconsdir}/swat.xpm

%clean
rm -rf $RPM_BUILD_ROOT

%post

/sbin/chkconfig --level 35 smb on

if [ -f /var/lock/subsys/xinetd ]; then
        service xinetd reload >/dev/null 2>&1 || :
fi

# Add a unix group for samba machine accounts
groupadd -frg 421 machines

# Migrate tdb's from /var/lock/samba (taken from official samba spec file):
for i in /var/lock/samba/*.tdb
do
if [ -f $i ]; then
	newname=`echo $i | sed -e's|var\/lock\/samba|var\/cache\/samba|'`
	echo "Moving $i to $newname"
	mv $i $newname
fi
done

# Remove the transient tdb files (modified from version in off. samba spec:
for TDB in brlock unexpected locking messages; do
        if [ -e /var/cache/samba/$TDB.tdb ]; then
                rm -f /var/cache/samba/$TDB.tdb;
        fi;
done

if [ -d /var/lock/samba ]; then
        rm -rf /var/lock/samba
fi

%post common
# Basic migration script for pre-2.2.1 users,
# since smb config moved from /etc to /etc/samba

mkdir -p /etc/samba
for s in smb.conf smbusers smbpasswd printers.def secrets.tdb lmhosts; do
[ -f /etc/$s ] && {
        cp -f /etc/$s /etc/$s.OLD
        mv -f /etc/$s /etc/samba/
}
done

# Let's create a proper /etc/samba/smbpasswd file
touch /etc/samba/smbpasswd

# Let's define the proper paths for config files
perl -pi -e 's/(\/etc\/)(smb)/\1samba\/\2/' /etc/samba/smb.conf

# Fix the logrotate.d file from smb and nmb to smbd and nmbd
if [ -f /etc/logrotate.d/samba ]; then
        perl -pi -e 's/smb /smbd /' /etc/logrotate.d/samba
        perl -pi -e 's/nmb /nmbd /' /etc/logrotate.d/samba
fi

# And not loose our machine account SIDs
[ -f /etc/*.SID ] && cp -f /etc/*.SID /etc/samba/

%update_menus

%preun

/sbin/chkconfig --level 35 smb reset

if [ $1 = 0 ] ; then

    for i in browse.dat wins.dat brlock.tdb unexpected.tdb connections.tdb \
locking.tdb messages.tdb;do
        if [ -e /var/cache/samba/$i ]; then
                rm -f /var/cache/samba/$i
        fi;
    done
    if [ -d /var/log/samba ]; then
      rm -rf /var/log/samba/*
    fi
    if [ -d /var/cache/samba ]; then
      rm -rf /var/cache/samba/*
    fi
fi

%preun common

if [ $1 = 0 ] ; then
    for n in /etc/samba/codepages/*; do
        if [ "$n" != "/etc/samba/codepages/src" ]; then
            rm -rf $n
        fi
    done
fi

%postun

# Remove swat entry from xinetd
if [ $1 = 0 -a -f /etc/xinetd.conf ] ; then
rm -f /etc/xinetd.d/swat
service xinetd reload &>/dev/null || :
fi

if [ "$1" = "0" -a -x /usr/bin/update-menus ]; then /usr/bin/update-menus || true ; fi

%clean_menus

%triggerpostun -- samba < 1.9.18p7

if [ $1 != 0 ]; then
    /sbin/chkconfig --level 35 smb on
fi

%triggerpostun -- samba < 2.0.5a-3, samba >= 2.0.0

if [ $1 != 0 ]; then
        [ ! -d /var/lock/samba ] && mkdir -m 0755 /var/lock/samba ||:
        [ ! -d /var/spool/samba ] && mkdir -m 1777 /var/spool/samba ||:
        [ -f /etc/inetd.conf ] && chmod 644 /etc/services /etc/inetd.conf ||:
fi

%files
%defattr(-,root,root)
%config(noreplace) /etc/xinetd.d/swat
%{_menudir}/%{name}
%{_miconsdir}/*.xpm
%{_liconsdir}/*.xpm
%{_iconsdir}/*.xpm
#%attr(-,root,root) %{prefix}/sbin/*
%attr(-,root,root) /sbin/*
#%attr(-,root,root) %{prefix}/bin/*
#%attr(755,root,root) /lib/*
%{prefix}/sbin/samba
%{prefix}/sbin/smbd
%{prefix}/sbin/nmbd
%{prefix}/sbin/swat
%{prefix}/sbin/smbcontrol
#%{prefix}/bin/addtosmbpass
%{prefix}/bin/mksmbpasswd.sh
%{prefix}/bin/smbstatus
%{prefix}/bin/smbpasswd
%{prefix}/bin/convert_smbpasswd
#/usr/share/swat
%attr(-,root,root) %{prefix}/share/swat/help/*
%attr(-,root,root) %{prefix}/share/swat/images/*
%attr(-,root,root) %{prefix}/share/swat/include/*
%attr(-,root,root) %config(noreplace) /etc/samba/smbusers
%attr(-,root,root) %config /etc/rc.d/init.d/smb
%attr(-,root,root) %config(noreplace) /etc/logrotate.d/samba
%attr(-,root,root) %config(noreplace) /etc/pam.d/samba
%{_mandir}/man1/smbstatus.1*
%{_mandir}/man5/smbpasswd.5*
%{_mandir}/man7/samba.7*
%{_mandir}/man8/smbd.8*
%{_mandir}/man8/nmbd.8*
%{_mandir}/man1/smbcontrol.1*
%{_mandir}/man8/smbpasswd.8*
%{_mandir}/man8/swat.8*
#%{_mandir}/man1/lmhosts.1*
%{_mandir}/man5/smb.conf.5*
%attr(775,root,root) %dir /var/lib/samba/netlogon
%attr(775,root,root) %dir /var/lib/samba/profiles
%attr(775,root,root) %dir /var/lib/samba/printers
%dir /var/cache/samba
%dir /var/log/samba
%attr(1777,root,root) %dir /var/spool/samba
%files doc
%defattr(-,root,root)
%doc README COPYING Manifest Read-Manifest-Now
%doc WHATSNEW.txt Roadmap
%doc docs
%doc examples
%doc swat/README
%attr(-,root,root) %{prefix}/share/swat/using_samba/*
%files client
%defattr(-,root,root)
%ifnarch alpha
/sbin/mount.smb
/sbin/mount.smbfs
%attr(4775,root,root) %{prefix}/bin/smbmount
%attr(4775,root,root) %{prefix}/bin/smbumount
%{prefix}/sbin/smbmnt
%{_mandir}/man8/smbmnt.8*
%{_mandir}/man8/smbmount.8*
%{_mandir}/man8/smbumount.8*
%endif
%{prefix}/bin/nmblookup
%{prefix}/bin/findsmb
%{prefix}/bin/smbclient
%{prefix}/bin/smbprint
%{prefix}/bin/smbtar
%{prefix}/bin/smbcacls
%{prefix}/bin/smbspool
# Link of smbspool to CUPS
/%{prefix}/lib/cups/backend/smb
/%{_mandir}/man1/nmblookup.1*
/%{_mandir}/man1/findsmb.1*
/%{_mandir}/man1/smbclient.1*
/%{_mandir}/man1/smbtar.1*
/%{_mandir}/man1/smbcacls.1*

%files common
%defattr(-,root,root)
/%{prefix}/bin/make_smbcodepage
/%{prefix}/bin/make_unicodemap
/%{prefix}/bin/testparm
/%{prefix}/bin/testprns
/%{prefix}/bin/make_printerdef
/%{prefix}/bin/rpcclient
/%{prefix}/bin/smbsh
%prefix/lib/smbwrapper.so
/lib/security/*
%attr(-,root,root) %config(noreplace) /etc/samba/smb.conf
%attr(-,root,root) %config(noreplace) /etc/samba/lmhosts
%attr(-,root,root) /var/lib/samba/codepages
%{_mandir}/man1/make_smbcodepage.1*
%{_mandir}/man1/make_unicodemap.1*
%{_mandir}/man1/testparm.1*
%{_mandir}/man1/smbsh.1*
%{_mandir}/man1/testprns.1*
%{_mandir}/man5/smb.conf.5*
%{_mandir}/man5/lmhosts.5*
%attr(755,root,root) /lib/*.so
%attr(755,root,root) /lib/*.so.*

%changelog
* Sat Jan 05 2002 John H Terpstra <jht@samba.org>
- Updated from Mandrake 8.1 SRPM to bring building up to date
- Note: I disposed of all patches that would not apply to the CVS sources
  no time to check if really needed - hope someone else will validate this.

* Mon Sep 10 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1a-15mdk
- Enabled acl support (XFS acls now supported by kernel-2.4.8-21mdk thx Chmou)
  Added smbd patch to support XFS quota (Nathan Scott)
  
* Mon Sep 10 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1a-14mdk
- Oops! smbpasswd created in wrong directory...

* Tue Sep 06 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1a-13mdk
- Removed a wrong comment in smb.conf.
  Added creation of smbpasswd during install.

* Mon Aug 27 2001 Pixel <pixel@mandrakesoft.com> 2.2.1a-12mdk
- really less verbose %%post

* Sat Aug 25 2001 Geoffrey Lee <snailtalk@mandrakesoft.com> 2.2.1a-11mdk
- Fix shared libs in /usr/bin silliness.

* Thu Aug 23 2001 Pixel <pixel@mandrakesoft.com> 2.2.1a-10mdk
- less verbose %%post

* Wed Aug 22 2001 Buchan Milne <bgmilne@cae.co.za> 2.2.1a-9mdk
- Added smbcacls (missing in %files), modification to smb.conf: ([printers]
  is still needed, even with point-and-print!, user add script should
  use name and not gid, since we may not get the gid . New script for
  putting manpages in place (still need to be added in %files!). Moved
  smbcontrol to sbin and added it and its man page to %files.

* Wed Aug 22 2001 Pixel <pixel@mandrakesoft.com> 2.2.1a-8mdk
- cleanup /var/lib/samba/codepage/src

* Tue Aug 21 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1a-7mdk
- moved codepage generation to %install and codepage dir to /var/lib/samba

* Tue Aug 21 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1a-6mdk
- /lib/* was in both samba and samba-common
  Introducing samba-doc: "alas, for the sake of thy modem, shalt thou remember
  when Samba was under the Megabyte..."

* Fri Aug 03 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1a-5mdk
- Added "the gc touch" to smbinit through the use of killall -0 instead of
  grep cupsd | grep -v grep (too many greps :o)

* Wed Jul 18 2001 Stefan van der Eijk <stefan@eijk.nu> 2.2.1a-4mdk
- BuildRequires: libcups-devel
- Removed BuildRequires: openssl-devel

* Fri Jul 13 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1a-3mdk
- replace chkconfig --add/del with --level 35 on/reset.

* Fri Jul 13 2001 Geoffrey Lee <snailtalk@mandrakesoft.cm> 2.2.1a-2mdk
- Replace discription s/inetd/xinetd/, we all love xinetd, blah.

* Thu Jul 12 2001 Buchan Milne <bgmilne@cae.co.za> 2.2.1a-1mdk
- Bugfix release. Fixed add user script, added print$ share and printer admin
  We need to test interaction of new print support with CUPS, but printer
  driver uploads should work.

* Wed Jul 11 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1-17mdk
- fixed smb.conf a bit, rebuilt on cooker.

* Tue Jul 10 2001 Buchan Milne <bgmilne@cae.co.za> 2.2.1-16mdk
- Finally, samba 2.2.1 has actually been release. At least we were ready!
  Cleaned up smb.conf, and added some useful entries for domain controlling.
  Migrated changes made in samba's samba2.spec for 2.2.1  to this file.
  Added groupadd command in post to create a group for samba machine accounts.
  (We should still check the postun, samba removes pam, logs and cache)

* Tue Jun 26 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1-15mdk
- fixed smbwrapper compile options.

* Tue Jun 26 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1-14mdk
- added LFS support.
  added smbwrapper support (smbsh)

* Wed Jun 20 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1-13mdk
- /sbin/mount.smb and /sbin/mount.smbfs now point to the correct location
  of smbmount (/usr/bin/smbmount)

* Tue Jun 19 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1-12mdk
- smbmount and smbumount are now in /usr/bin and SUID.
  added ||: to triggerpostun son you don't get error 1 anymore when rpm -e
  Checked the .bz2 sources with file *: everything is OK now (I'm so stupid ;o)!

* Tue Jun 19 2001 Geoffrey Lee <snailtalk@mandrakesoft.com> 2.2.1-11mdk
- s/Copyright/License/;
- Stop Sylvester from pretending .gz source to be .bz2 source via filename
  aka really bzip2 the source.

* Mon Jun 18 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1-10mdk
- changed Till's startup script modifications: now samba is being reloaded
  automatically 1 minute after it has started (same reasons as below in 9mdk)
  added _post_ and _preun_ for service smb
  fixed creation of /var/lib/samba/{netlogon,profiles} (%dir was missing)

* Thu Jun 14 2001 Till Kamppeter <till@mandrakesoft.com> 2.2.1-9mdk
- Modified the Samba startup script so that in case of CUPS being used as
  printing system Samba only starts when the CUPS daemon is ready to accept
  requests. Otherwise the CUPS queues would not appear as Samba shares.

* Mon Jun 11 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1-8mdk
- patched smbmount.c to have it call smbmnt in sbin (thanks Seb).

* Wed May 30 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1-7mdk
- put SWAT menu icons back in place.

* Mon May 28 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1-6mdk
- OOPS! fixed smbmount symlinks

* Mon May 28 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1-5mdk
- removed inetd postun script, replaced with xinetd.
  updated binary list (smbcacls...)
  cleaned samba.spec

* Mon May 28 2001 Buchan Milne <bgmilne@cae.co.za> 2.2.1-4mdk
- Changed configure options to point to correct log and codepage directories,
  added crude script to fix logrotate file for new log file names, updated
  patches to work with current CVS.

* Thu May 24 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1-3mdk
- Cleaned and updated the %files section.

* Sat May 19 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.1-2mdk
- Moved all samba files from /etc to /etc/samba (Thanks DomS!).
  Fixed fixinit patch (/etc/samba/smb.conf)

* Fri May 18 2001 Buchan Milne <bgmilne@cae.co.za> 2.2.1-1mdk
- Now use packaging/Mandrake/smb.conf, removed unused and obsolete
  patches, moved netlogon and profile shares to /var/lib/samba in the
  smb.conf to match the spec file. Added configuration for ntlogon to
  smb.conf. Removed pam-foo, fixinit and makefilepath patches. Removed
  symlink I introduced in 2.2.0-1mdk

* Thu May 3 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.0-5mdk
- Added more configure options. Changed Description field (thx John T).

* Wed Apr 25 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.0-4mdk
- moved netlogon and profiles to /var/lib/samba by popular demand ;o)

* Tue Apr 24 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.0-3mdk
- moved netlogon and profiles back to /home.

* Fri Apr 20 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.2.0-2mdk
- fixed post inetd/xinetd script&

* Thu Apr 19 2001 Buchan Milne <bgmilne@cae.co.za> 2.2.0-1mdk
- Upgrade to 2.2.0. Merged most of 2.0.7-25mdk's patches (beware
  nasty "ln -sf samba-%{ver} ../samba-2.0.7" hack to force some patches
  to take. smbadduser and addtosmbpass seem to have disappeared. Moved
  all Mandrake-specific files to packaging/Mandrake and made patches
  from those shipped with samba. Moved netlogon to /home/samba and added
  /home/samba/profiles. Added winbind,smbfilter and debug2html to make command.

* Thu Apr 12 2001 Frederic Crozat <fcrozat@mandrakesoft.com> 2.0.7-25mdk
- Fix menu entry and provide separate menu entry for GNOME
  (nautilus doesn't support HTTP authentication yet)
- Add icons in package

* Fri Mar 30 2001 Frederic Lepied <flepied@mandrakesoft.com> 2.0.7-24mdk
- use new server macros

* Wed Mar 21 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-23mdk
- check whether /etc/inetd.conf exists (upgrade) or not (fresh install).

* Thu Mar 15 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-22mdk
- spec cosmetics, added '-r' option to lpr-cups command line so files are
  removed from /var/spool/samba after printing.

* Tue Mar 06 2001 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-21mdk
- merged last rh patches.

* Thu Nov 23 2000 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-20mdk
- removed dependencies on cups and cups-devel so one can install samba without using cups
- added /home/netlogon

* Mon Nov 20 2000 Till Kamppeter <till@mandrakesoft.com> 2.0.7-19mdk
- Changed default print command in /etc/smb.conf, so that the Windows
  driver of the printer has to be used on the client.
- Fixed bug in smbspool which prevented from printing from a
  Linux-Samba-CUPS client to a Windows server through the guest account.

* Mon Oct 16 2000 Till Kamppeter <till@mandrakesoft.com> 2.0.7-18mdk
- Moved "smbspool" (Samba client of CUPS) to the samba-client package

* Sat Oct 7 2000 Stefan van der Eijk <s.vandereijk@chello.nl> 2.0.7-17mdk
- Added RedHat's "quota" patch to samba-glibc21.patch.bz2, this fixes
  quota related compile problems on the alpha.

* Wed Oct 4 2000 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-16mdk
- Fixed 'guest ok = ok' flag in smb.conf

* Tue Oct 3 2000 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-15mdk
- Allowed guest account to print in smb.conf
- added swat icon in menu

* Tue Oct 3 2000 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-14mdk
- Removed rh ssl patch and --with-ssl flag: not appropriate for 7.2

* Tue Oct 3 2000 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-13mdk
- Changed fixinit patch.
- Changed smb.conf for better CUPS configuration.
- Thanks Fred for doing this ---vvv.

* Tue Oct  3 2000 Frederic Lepied <flepied@mandrakesoft.com> 2.0.7-12mdk
- menu entry for web configuration tool.
- merge with rh: xinetd + ssl + pam_stack.
- Added smbadduser rh-bugfix w/o relocation of config-files.

* Mon Oct  2 2000 Frederic Lepied <flepied@mandrakesoft.com> 2.0.7-11mdk
- added build requires on cups-devel and pam-devel.

* Mon Oct  2 2000 Till Kamppeter <till@mandrakesoft.com> 2.0.7-10mdk
- Fixed smb.conf entry for CUPS: "printcap name = lpstat", "lpstats" was
  wrong.

* Mon Sep 25 2000 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-9mdk
- Cosmetic changes to make rpmlint more happy

* Wed Sep 11 2000 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-8mdk
- added linkage to the using_samba book in swat

* Fri Sep 01 2000 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-7mdk
- Added CUPS support to smb.conf
- Added internationalization options to smb.conf [Global]

* Wed Aug 30 2000 Till Kamppeter <till@mandrakesoft.com> 2.0.7-6mdk
- Put "smbspool" to the files to install

* Wed Aug 30 2000 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-5mdk
- Did some cleaning in the patches

* Fri Jul 28 2000 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-4mdk
- relocated man pages from /usr/man to /usr/share/man for compatibility reasons

* Fri Jul 28 2000 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-3mdk
- added make_unicodemap and build of unicode_map.$i in the spec file

* Fri Jul 28 2000 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-2mdk
- renamed /etc/codepage/codepage.$i into /etc/codepage/unicode_map.$i to fix smbmount bug.

* Fri Jul 07 2000 Sylvestre Taburet <staburet@mandrakesoft.com> 2.0.7-1mdk
- 2.0.7

* Wed Apr 05 2000 Francis Galiegue <fg@mandrakesoft.com> 2.0.6-4mdk

- Titi sucks, does not put versions in changelog
- Fixed groups for -common and -client
- /usr/sbin/samba is no config file

* Thu Mar 23 2000 Thierry Vignaud <tvignaud@mandrakesoft.com>
- fix buggy post install script (pixel)

* Fri Mar 17 2000 Francis Galiegue <francis@mandrakesoft.com> 2.0.6-2mdk

- Changed group according to 7.1 specs
- Some spec file changes
- Let spec-helper do its job

* Thu Nov 25 1999 Chmouel Boudjnah <chmouel@mandrakesoft.com>
- 2.0.6.

* Tue Nov  2 1999 Chmouel Boudjnah <chmouel@mandrakesoft.com>
- Merge with rh changes.
- Split in 3 packages.

* Fri Aug 13 1999 Pablo Saratxaga <pablo@@mandrakesoft.com>
- corrected a bug with %post (the $1 parameter is "1" in case of
  a first install, not "0". That parameter is the number of packages
  of the same name that will exist after running all the steps if nothing
  is removed; so it is "1" after first isntall, "2" for a second install
  or an upgrade, and "0" for a removal)

* Wed Jul 28 1999 Pablo Saratxaga <pablo@@mandrakesoft.com>
- made smbmnt and smbumount suid root, and only executable by group 'smb'
  add to 'smb' group any user that should be allowed to mount/unmount
  SMB shared directories

* Fri Jul 23 1999 Chmouel Boudjnah <chmouel@mandrakesoft.com>
- 2.0.5a (bug security fix).

* Wed Jul 21 1999 Axalon Bloodstone <axalon@linux-mandrake.com>
- 2.0.5
- cs/da/de/fi/fr/it/tr descriptions/summaries

* Sun Jun 13 1999 Bernhard Rosenkränzer <bero@mandrakesoft.com>
- 2.0.4b
- recompile on a system that works ;)

* Wed Apr 21 1999 Chmouel Boudjnah <chmouel@mandrakesoft.com>
- Mandrake adaptations.
- Bzip2 man-pages.

* Fri Mar 26 1999 Bill Nottingham <notting@redhat.com>
- add a mount.smb to make smb mounting a little easier.
- smb filesystems apparently do not work on alpha. Oops.

* Thu Mar 25 1999 Bill Nottingham <notting@redhat.com>
- always create codepages

* Tue Mar 23 1999 Bill Nottingham <notting@redhat.com>
- logrotate changes

* Sun Mar 21 1999 Cristian Gafton <gafton@redhat.com>
- auto rebuild in the new build environment (release 3)

* Fri Mar 19 1999 Preston Brown <pbrown@redhat.com>
- updated init script to use graceful restart (not stop/start)

* Tue Mar  9 1999 Bill Nottingham <notting@redhat.com>
- update to 2.0.3

* Thu Feb 18 1999 Bill Nottingham <notting@redhat.com>
- update to 2.0.2

* Mon Feb 15 1999 Bill Nottingham <notting@redhat.com>
- swat swat

* Tue Feb  9 1999 Bill Nottingham <notting@redhat.com>
- fix bash2 breakage in post script

* Fri Feb  5 1999 Bill Nottingham <notting@redhat.com>
- update to 2.0.0

* Mon Oct 12 1998 Cristian Gafton <gafton@redhat.com>
- make sure all binaries are stripped

* Thu Sep 17 1998 Jeff Johnson <jbj@redhat.com>
- update to 1.9.18p10.
- fix %triggerpostun.

* Tue Jul 07 1998 Erik Troan <ewt@redhat.com>
- updated postun triggerscript to check $0
- clear /etc/codepages from %preun instead of %postun

* Mon Jun 08 1998 Erik Troan <ewt@redhat.com>
- made the %postun script a tad less agressive; no reason to remove
  the logs or lock file (after all, if the lock file is still there,
  samba is still running)
- the %postun and %preun should only exectute if this is the final
  removal
- migrated %triggerpostun from Red Hat's samba package to work around
  packaging problems in some Red Hat samba releases

* Sun Apr 26 1998 John H Terpstra <jht@samba.anu.edu.au>
- minor tidy up in preparation for release of 1.9.18p5
- added findsmb utility from SGI package

* Wed Mar 18 1998 John H Terpstra <jht@samba.anu.edu.au>
- Updated version and codepage info.
- Release to test name resolve order

* Sat Jan 24 1998 John H Terpstra <jht@samba.anu.edu.au>
- Many optimisations (some suggested by Manoj Kasichainula <manojk@io.com>
- Use of chkconfig in place of individual symlinks to /etc/rc.d/init/smb
- Compounded make line
- Updated smb.init restart mechanism
- Use compound mkdir -p line instead of individual calls to mkdir
- Fixed smb.conf file path for log files
- Fixed smb.conf file path for incoming smb print spool directory
- Added a number of options to smb.conf file
- Added smbadduser command (missed from all previous RPMs) - Doooh!
- Added smbuser file and smb.conf file updates for username map
