<?php # $Id: serendipity_event_weblogping.php 235 2005-07-08 13:29:39Z garvinhicking $

        @define('PLUGIN_EVENT_WEBLOGPING_PING', 'Eintr�ge ank�ndigen (via XML-RPC ping) bei:');
        @define('PLUGIN_EVENT_WEBLOGPING_SENDINGPING', 'Sende XML-RPC ping zu %s');
        @define('PLUGIN_EVENT_WEBLOGPING_TITLE', 'Eintr�ge ank�ndigen');
        @define('PLUGIN_EVENT_WEBLOGPING_DESC', 'Benachrichtigt diverse Internetseiten, das ein neuer Eintrag erstellt wurde.');
        @define('PLUGIN_EVENT_WEBLOGPING_SUPERSEDES', '(ersetzt %s)');
        @define('PLUGIN_EVENT_WEBLOGPING_CUSTOM', 'Selbstdefinierte Ping-Services');
        @define('PLUGIN_EVENT_WEBLOGPING_CUSTOM_BLAHBLA', 'Mehrere durch "," getrennte Ping-Services im Format: "host.domain/pfad". Falls am Anfang eines Hosts ein "*" eingef�gt wird, werden an den Host die erweiterten XML-RPC Optionen gesendet; der Host muss diese Optionen unterst�tzen.');
