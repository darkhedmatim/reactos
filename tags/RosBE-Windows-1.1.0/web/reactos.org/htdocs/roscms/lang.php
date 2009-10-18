<?php

// Language detection
function check_lang($lang)
{
	if (preg_match('/^([a-zA-Z]+)(-[a-zA-Z]+)?$/', $lang, $matches)) {
		$checked_lang = strtolower($matches[1]);
		switch($checked_lang) {
		case 'de':
		case 'en':
		case 'fr':
		case 'ru':
		case 'es':
		case 'it':
		case 'hu':
		case 'sv':
		case 'lt':
		case 'nl':
		case 'pl':
		case 'no':
		case 'da':
		case 'id':
		case 'zh':
			break;
		default:
			$checked_lang = '';
		}
	}
	else if ($lang == '*') {
		$checked_lang = 'en';
	}
	else {
		$checked_lang = '';
	}

	return $checked_lang;
}

if ($rpm_lang == '' && isset($_COOKIE['roscms_usrset_lang'])) {
	$rpm_lang = $_COOKIE['roscms_usrset_lang'];
	if (substr($rpm_lang, -1) == '/') {
		$rpm_lang = substr($rpm_lang, strlen($rpm_lang) - 1);
	}
	$rpm_lang = check_lang($rpm_lang);
}

if ($rpm_lang == '') {
	/* After parameter and cookie processing, we still don't have a valid
           language. So check whether the HTTP Accept-language header can
           help us. */
	$accept_language = $_SERVER['HTTP_ACCEPT_LANGUAGE'];
	$best_q = 0;
	while (preg_match('/^\s*([^,]+)((,(.*))|$)/',
	                  $accept_language, $matches)) {
		$lang_range = @$matches[1];
		$accept_language = @$matches[4];
		if (preg_match('/^(([a-zA-Z]+)(-[a-zA-Z]+)?)(;q=([0-1](\.[0-9]{1,3})?))?/',
		               $lang_range, $matches)) {
			$lang = check_lang($matches[1]);
			if ($lang != '') {
				$q = @$matches[5];
				if ($q == "") {
					$q = 1;
				}
				else {
					settype($q, 'float');
				}
				if ($best_q < $q) {
					$rpm_lang = $lang;
					$best_q = $q;
				}
			}
		}
	}
}
if ($rpm_lang == '') {
	/* If all else fails, use the default language */
	$rpm_lang = check_lang('*');
}

$roscms_page_lang = $rpm_lang . '/';
$rpm_lang_session = $rpm_lang . '/';


if (isset($_COOKIE['roscms_usrset_lang']) || isset($_REQUEST['lang'])) {
	/* Delete an existing cookie (if any) which uses the full hostname */
	setcookie('roscms_usrset_lang', '', -3600);
	/* Add cookie using just the domain name */
	require_once('inc/utils.php');
	setcookie('roscms_usrset_lang', $rpm_lang, time() + 5 * 30 * 24 * 3600,
	          '/', cookie_domain());
}


	require("inc/lang/en.php"); // preload the english language text
	require("inc/lang/".$rpm_lang.".php"); // load the and overwrite the language text


?>