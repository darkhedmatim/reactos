<?php # $Id: serendipity_event_statistics.php 310 2005-07-28 01:10:43Z wesley $

        @define('PLUGIN_EVENT_STATISTICS_NAME', '����������');
        @define('PLUGIN_EVENT_STATISTICS_DESC', '������ ������ ��� ��������� ������������� ����� � ���������� �� ����������, ����������� � ����� �� ������������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_STATISTICS', '����������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_FIRST_ENTRY', '����� �������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_LAST_ENTRY', '�������� �������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOTAL_ENTRIES', '��� ���� ��������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ENTRIES', '�������(�)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOTAL_PUBLIC', ' ... �����������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOTAL_DRAFTS', ' ... ������� (drafts)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_CATEGORIES', '���������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_CATEGORIES2', '���������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_CATEGORIES', '������������� �� ����������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_CATEGORIES2', '�������(�)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_UPLOADED_IMAGES', '�������� (uploaded) ��������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_UPLOADED_IMAGES2', '��������');
		@define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_IMAGES', '������������� �� �������� ��������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_IMAGES2', '�����');
        @define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS', '�������� ���������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS2', '��������(�)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS3', '���-����������� ��������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOPCOMMENTS', '���-����� ����������� ����������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_LINK', '������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_SUBSCRIBERS', '�������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_SUBSCRIBERS2', '������(�)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOPSUBSCRIBERS', '�������� � ���-����� �������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOPSUBSCRIBERS2', '������(�)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS', '���������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS2', '��������(�)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACK', '�������� � ���-����� ���������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACK2', '��������(�)');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACKS3', '���-����� ���������� �������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS_PER_ARTICLE', '������ ��������� �� �������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS_PER_ARTICLE', '������ ���������� �� �������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_DAY', '������ �������� �� ���');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_WEEK', '������ �������� �� �������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_MONTH', '������ �������� �� �����');
        @define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS_PER_ARTICLE2', '���������/�������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS_PER_ARTICLE2', '����������/�������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_DAY2', '��������/���');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_WEEK2', '��������/�������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_MONTH2', '��������/�����');
        @define('PLUGIN_EVENT_STATISTICS_OUT_CHARS', '��� ���� �� ���������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_CHARS2', '�������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_CHARS_PER_ARTICLE', '������ ������� � �������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_CHARS_PER_ARTICLE2', '�������/�������');
        @define('PLUGIN_EVENT_STATISTICS_OUT_LONGEST_ARTICLES', '%s-�� ���-����� ��������');
        @define('PLUGIN_EVENT_STATISTICS_MAX_ITEMS', '���������� ���� �����');
        @define('PLUGIN_EVENT_STATISTICS_MAX_ITEMS_DESC', '����� ����� �� �� ������� �� ������ �������������� �������� (�� ������������: 20)');

//Language constants for the Extended Visitors feature
		@define('PLUGIN_EVENT_STATISTICS_EXT_ADD', '������������ ���������� �� ������������');
		@define('PLUGIN_EVENT_STATISTICS_EXT_ADD_DESC', '�� �� ������ �� ���������� �� ��������� �� ������������ ����� �� ������������ (�� ������������: ��)');
		@define('PLUGIN_EVENT_STATISTICS_EXT_OPT1', ' ��');
		@define('PLUGIN_EVENT_STATISTICS_EXT_OPT2', '��, ���� �� ����������');
		@define('PLUGIN_EVENT_STATISTICS_EXT_OPT3', '��, ���� �� ����������');
		@define('PLUGIN_EVENT_STATISTICS_EXT_VISITORS', '���� �� ������������');
		@define('PLUGIN_EVENT_STATISTICS_EXT_VISTODAY', '���� �� ������������ ����');
		@define('PLUGIN_EVENT_STATISTICS_EXT_VISTOTAL', '��� ���� �� ������������');
		@define('PLUGIN_EVENT_STATISTICS_EXT_VISSINCE', '������������ ������������� ����� �� ������������ �� ������� ��');
		@define('PLUGIN_EVENT_STATISTICS_EXT_VISLATEST', '�������� ����������');
		@define('PLUGIN_EVENT_STATISTICS_EXT_TOPREFS', '���-����� ���������� �������');
		@define('PLUGIN_EVENT_STATISTICS_EXT_TOPREFS_NONE', '��������� �� �� ����������� �� ����.');
		@define('PLUGIN_EVENT_STATISTICS_OUT_EXT_STATISTICS', '������������ ���������� �� ������������');
		@define('PLUGIN_EVENT_STATISTICS_BANNED_HOSTS', '�������, ����� �� ������ �� ����� ������');
		@define('PLUGIN_EVENT_STATISTICS_BANNED_HOSTS_DESC', '�������� ���������, ����� ������ �� ����� ��������� �� �����������, ��������� � "|"');






