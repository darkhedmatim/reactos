<?php # $Id: serendipity_event_mailer.php 283 2005-07-22 08:56:47Z garvinhicking $

        @define('PLUGIN_EVENT_MAILER_NAME', 'Artikel mailen');
        @define('PLUGIN_EVENT_MAILER_DESC', 'Verschickt jeden neuen Artikel im Blog via E-Mail');
        @define('PLUGIN_EVENT_MAILER_RECIPIENT', 'Mail-Empf�nger');
        @define('PLUGIN_EVENT_MAILER_RECIPIENTDESC', 'Die E-Mail Adresse an die die Artikel verschickt werden sollen (empfohlen: Eine Mailing-Liste)');
        @define('PLUGIN_EVENT_MAILER_LINK', 'URL des Artikels mailen?');
        @define('PLUGIN_EVENT_MAILER_LINKDESC', 'Verschickt die URL des Artikels.');
        @define('PLUGIN_EVENT_MAILER_STRIPTAGS', 'HTML entfernen?');
        @define('PLUGIN_EVENT_MAILER_STRIPTAGSDESC', 'Entfernt HTML-Anweisungen aus der Mail.');
        @define('PLUGIN_EVENT_MAILER_CONVERTP', 'HTML-Paragraphen in Leerzeilen wandeln?');
        @define('PLUGIN_EVENT_MAILER_CONVERTPDESC', 'F�gt einen Zeilenumbruch nach jedem HTML-Paragraphen ein. Besonders hilfreich im Zusammenhang mit der Option "HTML Entfernen", damit der Eintrag sp�ter selbst dann Umbr�che enth�lt, wenn sie nicht manuell eingegeben wurden.');
