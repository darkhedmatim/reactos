<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<group>
<directory name="crt">
	<xi:include href="crt/crt.xml" />
</directory>
<directory name="dxguid">
	<xi:include href="dxguid/dxguid.xml" />
</directory>
<directory name="epsapi">
	<xi:include href="epsapi/epsapi.xml" />
</directory>
<directory name="fslib">
	<xi:include href="fslib/directory.xml" />
</directory>
<directory name="inflib">
	<xi:include href="inflib/inflib.xml" />
</directory>
<directory name="intrlck">
	<xi:include href="intrlck/intrlck.xml" />
</directory>
<directory name="kjs">
	<xi:include href="kjs/kjs.xml" />
</directory>
<directory name="libwine">
	<xi:include href="libwine/libwine.xml" />
</directory>
<directory name="nt">
	<xi:include href="nt/nt.xml" />
</directory>
<directory name="pseh">
	<xi:include href="pseh/pseh.xml" />
</directory>
<directory name="rtl">
	<xi:include href="rtl/rtl.xml" />
</directory>
<directory name="smdll">
	<xi:include href="smdll/smdll.xml" />
</directory>
<directory name="smlib">
	<xi:include href="smlib/smlib.xml" />
</directory>
<directory name="string">
	<xi:include href="string/string.xml" />
</directory>
<directory name="strmiids">
	<xi:include href="strmiids/strmiids.xml" />
</directory>
<directory name="uuid">
	<xi:include href="uuid/uuid.xml" />
</directory>
<directory name="wdmguid">
	<xi:include href="wdmguid/wdmguid.xml" />
</directory>
</group>

<!-- The following entires got moved:
<directory name="acledit">
	<xi:include href="acledit/acledit.xml" />
</directory>
<directory name="aclui">
	<xi:include href="aclui/aclui.xml" />
</directory>
<directory name="adns">
	<xi:include href="adns/adns.xml" />
</directory>
<directory name="advapi32">
	<xi:include href="advapi32/advapi32.xml" />
</directory>
<directory name="advpack">
	<xi:include href="advpack/advpack.xml" />
</directory>
<directory name="authz">
	<xi:include href="authz/authz.xml" />
</directory>
<directory name="avifil32">
	<xi:include href="avifil32/avifil32.xml" />
</directory>
<directory name="cabinet">
	<xi:include href="cabinet/cabinet.xml" />
</directory>
<directory name="cards">
	<xi:include href="cards/cards.xml" />
</directory>
<directory name="cfgmgr32">
	<xi:include href="cfgmgr32/cfgmgr32.xml" />
</directory>
<directory name="comctl32">
	<xi:include href="comctl32/comctl32.xml" />
</directory>
<directory name="comdlg32">
	<xi:include href="comdlg32/comdlg32.xml" />
</directory>
<directory name="cpl">
	<xi:include href="cpl/directory.xml" />
</directory>
<directory name="crtdll">
	<xi:include href="crtdll/crtdll.xml" />
</directory>
<directory name="crypt32">
	<xi:include href="crypt32/crypt32.xml" />
</directory>
<directory name="d3d8thk">
	<xi:include href="d3d8thk/d3d8thk.xml" />
</directory>
<directory name="dbghelp">
	<xi:include href="dbghelp/dbghelp.xml" />
</directory>
<directory name="ddraw">
	<xi:include href="ddraw/ddraw.xml" />
</directory>
<directory name="devenum">
	<xi:include href="devenum/devenum.xml" />
</directory>
<directory name="devmgr">
	<xi:include href="devmgr/devmgr.xml" />
</directory>
<directory name="dhcpcsvc">
	<xi:include href="dhcpcsvc/dhcpcsvc.xml" />
</directory>
<directory name="dinput">
	<xi:include href="dinput/dinput.xml" />
</directory>
<directory name="dinput8">
	<xi:include href="dinput8/dinput8.xml" />
</directory>
<directory name="dnsapi">
	<xi:include href="dnsapi/dnsapi.xml" />
</directory>
<directory name="dplay">
	<xi:include href="dplay/dplay.xml" />
</directory>
<directory name="dplayx">
	<xi:include href="dplayx/dplayx.xml" />
</directory>
<directory name="dsound">
	<xi:include href="dsound/dsound.xml" />
</directory>
<directory name="dxdiagn">
	<xi:include href="dxdiagn/dxdiagn.xml" />
</directory>
<directory name="expat">
	<xi:include href="expat/expat.xml" />
</directory>
<directory name="fmifs">
	<xi:include href="fmifs/fmifs.xml" />
</directory>
<directory name="freetype">
	<xi:include href="freetype/freetype.xml" />
</directory>
<directory name="gdi32">
	<xi:include href="gdi32/gdi32.xml" />
</directory>
<directory name="gdiplus">
	<xi:include href="gdiplus/gdiplus.xml" />
</directory>
<directory name="glu32">
	<xi:include href="glu32/glu32.xml" />
</directory>
<directory name="hid">
	<xi:include href="hid/hid.xml" />
</directory>
<directory name="icmp">
	<xi:include href="icmp/icmp.xml" />
</directory>
<directory name="imagehlp">
	<xi:include href="imagehlp/imagehlp.xml" />
</directory>
<directory name="imm32">
	<xi:include href="imm32/imm32.xml" />
</directory>
<directory name="iphlpapi">
	<xi:include href="iphlpapi/iphlpapi.xml" />
</directory>
<directory name="keyboard">
	<xi:include href="keyboard/directory.xml" />
</directory>
<directory name="kernel32">
	<xi:include href="kernel32/kernel32.xml" />
</directory>
<directory name="lsasrv">
	<xi:include href="lsasrv/lsasrv.xml" />
</directory>
<directory name="lzexpand">
	<xi:include href="lzexpand/lz32.xml" />
</directory>
<directory name="mapi32">
	<xi:include href="mapi32/mapi32.xml" />
</directory>
<directory name="mesa32">
	<xi:include href="mesa32/mesa32.xml" />
</directory>
<directory name="mmdrv">
	<xi:include href="mmdrv/mmdrv.xml" />
</directory>
<directory name="mpr">
	<xi:include href="mpr/mpr.xml" />
</directory>
<directory name="msacm">
	<xi:include href="msacm/msacm32.xml" />
</directory>
<directory name="msafd">
	<xi:include href="msafd/msafd.xml" />
</directory>
<directory name="msgina">
	<xi:include href="msgina/msgina.xml" />
</directory>
<directory name="msi">
	<xi:include href="msi/msi.xml" />
</directory>
<directory name="msimg32">
	<xi:include href="msimg32/msimg32.xml" />
</directory>
<directory name="msvcrt">
	<xi:include href="msvcrt/msvcrt.xml" />
</directory>
<directory name="msvcrt20">
	<xi:include href="msvcrt20/msvcrt20.xml" />
</directory>
<directory name="msvideo">
	<xi:include href="msvideo/msvfw32.xml" />
</directory>
<directory name="mswsock">
	<xi:include href="mswsock/mswsock.xml" />
</directory>
<directory name="netapi32">
	<xi:include href="netapi32/netapi32.xml" />
</directory>
<directory name="netcfgx">
	<xi:include href="netcfgx/netcfgx.xml" />
</directory>
<directory name="newdev">
	<xi:include href="newdev/newdev.xml" />
</directory>
<directory name="ntdll">
	<xi:include href="ntdll/ntdll.xml" />
</directory>
<directory name="ntmarta">
	<xi:include href="ntmarta/ntmarta.xml" />
</directory>
<directory name="objsel">
	<xi:include href="objsel/objsel.xml" />
</directory>
<directory name="ole32">
	<xi:include href="ole32/ole32.xml" />
</directory>
<directory name="oleacc">
	<xi:include href="oleacc/oleacc.xml" />
</directory>
<directory name="oleaut32">
	<xi:include href="oleaut32/oleaut32.xml" />
</directory>
<directory name="oledlg">
	<xi:include href="oledlg/oledlg.xml" />
</directory>
<directory name="olepro32">
	<xi:include href="olepro32/olepro32.xml" />
</directory>
<directory name="opengl32">
	<xi:include href="opengl32/opengl32.xml" />
</directory>
<directory name="psapi">
	<xi:include href="psapi/psapi.xml" />
</directory>
<directory name="richedit">
	<xi:include href="richedit/riched32.xml" />
</directory>
<directory name="riched20">
	<xi:include href="riched20/riched20.xml" />
</directory>
<directory name="rossym">
	<xi:include href="rossym/rossym.xml" />
</directory>
<directory name="rpcrt4">
	<xi:include href="rpcrt4/rpcrt4.xml" />
</directory>
<directory name="samlib">
	<xi:include href="samlib/samlib.xml" />
</directory>
<directory name="samsrv">
	<xi:include href="samsrv/samsrv.xml" />
</directory>
<directory name="secur32">
	<xi:include href="secur32/secur32.xml" />
</directory>
<directory name="security">
	<xi:include href="security/security.xml" />
</directory>
<directory name="serialui">
	<xi:include href="serialui/serialui.xml" />
</directory>
<directory name="setupapi">
	<xi:include href="setupapi/setupapi.xml" />
</directory>
<directory name="shdocvw">
	<xi:include href="shdocvw/shdocvw.xml" />
</directory>
<directory name="shell32">
	<xi:include href="shell32/shell32.xml" />
</directory>
<directory name="shellext">
	<xi:include href="shellext/directory.xml" />
</directory>
<directory name="shfolder">
	<xi:include href="shfolder/shfolder.xml" />
</directory>
<directory name="shlwapi">
	<xi:include href="shlwapi/shlwapi.xml" />
</directory>
<directory name="snmpapi">
	<xi:include href="snmpapi/snmpapi.xml" />
</directory>
<directory name="syssetup">
	<xi:include href="syssetup/syssetup.xml" />
</directory>
<directory name="twain">
	<xi:include href="twain/twain_32.xml" />
</directory>
<directory name="urlmon">
	<xi:include href="urlmon/urlmon.xml" />
</directory>
<directory name="user32">
	<xi:include href="user32/user32.xml" />
</directory>
<directory name="userenv">
	<xi:include href="userenv/userenv.xml" />
</directory>
<directory name="uxtheme">
	<xi:include href="uxtheme/uxtheme.xml" />
</directory>
<directory name="vdmdbg">
	<xi:include href="vdmdbg/vdmdbg.xml" />
</directory>
<directory name="version">
	<xi:include href="version/version.xml" />
</directory>
<directory name="wininet">
	<xi:include href="wininet/wininet.xml" />
</directory>
<directory name="winmm">
	<xi:include href="winmm/directory.xml" />
</directory>
<directory name="winspool">
	<xi:include href="winspool/winspool.xml" />
</directory>
<directory name="wintrust">
	<xi:include href="wintrust/wintrust.xml" />
</directory>
<directory name="ws2_32">
	<xi:include href="ws2_32/ws2_32.xml" />
</directory>
<directory name="ws2help">
	<xi:include href="ws2help/ws2help.xml" />
</directory>
<directory name="wshirda">
	<xi:include href="wshirda/wshirda.xml" />
</directory>
<directory name="wsock32">
	<xi:include href="wsock32/wsock32.xml" />
</directory>
<directory name="zlib">
	<xi:include href="zlib/zlib.xml" />
</directory>
-->
