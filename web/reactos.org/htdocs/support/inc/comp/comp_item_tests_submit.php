<?php
    /*
    RSDB - ReactOS Support Database
    Copyright (C) 2005-2006  Klemens Friedl <frik85@reactos.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    */

/*
 *	ReactOS Support Database System - RSDB
 *	
 *	(c) by Klemens Friedl <frik85>
 *	
 *	2005 - 2006 
 */


	// To prevent hacking activity:
	if ( !defined('RSDB') )
	{
		die(" ");
	}


$query_page = mysql_query("SELECT * 
							FROM `rsdb_item_comp` 
							WHERE `comp_visible` = '1'
							AND `comp_id` = " . mysql_real_escape_string($RSDB_SET_item) . "
							ORDER BY `comp_name` ASC") ;

$result_page = mysql_fetch_array($query_page);		


	echo "<h2>".htmlentities($result_page['comp_name']) ." [". "ReactOS ".show_osversion($result_page['comp_osversion']) ."]</h2>"; 


if ($RSDB_intern_user_id <= 0) {
	please_register(); 
}
else {
	
	include('inc/tree/tree_item_menubar.php');
	
	echo "<h3>Submit a Report</h3>";

	$RSDB_TEMP_SUBMIT_valid = true;
	
	$RSDB_TEMP_submitpost = "";
	$RSDB_TEMP_txtwhatwork = "";
	$RSDB_TEMP_txtwhatnot = "";
	$RSDB_TEMP_txtwhatnottested = "";
	$RSDB_TEMP_txtcomment = "";
	$RSDB_TEMP_optfunc = "";
	$RSDB_TEMP_optinstall = "";
	$RSDB_TEMP_txtconclusion = "";
	$RSDB_TEMP_txtrevision = "";
	
	if (array_key_exists("submitpost", $_POST)) $RSDB_TEMP_submitpost=htmlspecialchars($_POST["submitpost"]);
	if (array_key_exists("txtwhatwork", $_POST)) $RSDB_TEMP_txtwhatwork=htmlspecialchars($_POST["txtwhatwork"]);
	if (array_key_exists("txtwhatnot", $_POST)) $RSDB_TEMP_txtwhatnot=htmlspecialchars($_POST["txtwhatnot"]);
	if (array_key_exists("txtwhatnottested", $_POST)) $RSDB_TEMP_txtwhatnottested=htmlspecialchars($_POST["txtwhatnottested"]);
	if (array_key_exists("txtcomment", $_POST)) $RSDB_TEMP_txtcomment=htmlspecialchars($_POST["txtcomment"]);
	if (array_key_exists("optfunc", $_POST)) $RSDB_TEMP_optfunc=htmlspecialchars($_POST["optfunc"]);
	if (array_key_exists("optinstall", $_POST)) $RSDB_TEMP_optinstall=htmlspecialchars($_POST["optinstall"]);
	if (array_key_exists("txtconclusion", $_POST)) $RSDB_TEMP_txtconclusion=htmlspecialchars($_POST["txtconclusion"]);
	if (array_key_exists("txtrevision", $_POST)) $RSDB_TEMP_txtrevision=htmlspecialchars($_POST["txtrevision"]);

	if ($RSDB_TEMP_submitpost == "yes") {
		if (strlen($RSDB_TEMP_txtwhatwork) <= 3) {
			msg_bar("The 'What works' textbox is (almost) empty  ...");
			echo "<br />";
			$RSDB_TEMP_SUBMIT_valid = false;
		}
		/*if (strlen($RSDB_TEMP_txtwhatnot) <= 3) {
			msg_bar("The 'What does not work' textbox is (almost) empty  ...");
			$RSDB_TEMP_SUBMIT_valid = false;
			echo "<br />";
		}*/
		if (strlen($RSDB_TEMP_txtwhatnottested) <= 3) {
			msg_bar("The 'What has been tested and what not' textbox is (almost) empty  ...");
			$RSDB_TEMP_SUBMIT_valid = false;
			echo "<br />";
		}
		if ($RSDB_TEMP_optfunc < 1 || $RSDB_TEMP_optfunc > 5) {
			msg_bar("Application function: please select the star(s) ...");
			$RSDB_TEMP_SUBMIT_valid = false;
			echo "<br />";
		}
		if ($RSDB_TEMP_optinstall < 1 || $RSDB_TEMP_optinstall > 5) {
			msg_bar("Installation routine: please select the star(s) ...");
			$RSDB_TEMP_SUBMIT_valid = false;
			echo "<br />";
		}
		if (strlen($RSDB_TEMP_txtconclusion) <= 3) {
			msg_bar("The 'Conclusion' textbox is (almost) empty  ...");
			$RSDB_TEMP_SUBMIT_valid = false;
			echo "<br />";
		}
	}
	if ($RSDB_TEMP_SUBMIT_valid == "yes" && $RSDB_TEMP_submitpost == true) {
		$report_submit="INSERT INTO `rsdb_item_comp_testresults` ( `test_id` , `test_comp_id` , `test_visible` , `test_whatworks` , `test_whatdoesntwork` , `test_whatnottested` , `test_date` , `test_result_install` , `test_result_function` , `test_user_comment` , `test_conclusion` , `test_user_id` , `test_user_submit_timestamp` , `test_useful_vote_value` , `test_useful_vote_user` , `test_useful_vote_user_history` , `test_com_version` ) 
						VALUES ('', '".mysql_real_escape_string($RSDB_SET_item)."', '1', '".mysql_real_escape_string($RSDB_TEMP_txtwhatwork)."', '".mysql_real_escape_string($RSDB_TEMP_txtwhatnot)."', '".mysql_real_escape_string($RSDB_TEMP_txtwhatnottested)."', NOW( ), '".mysql_real_escape_string($RSDB_TEMP_optinstall)."', '".mysql_real_escape_string($RSDB_TEMP_optfunc)."', '".mysql_real_escape_string($RSDB_TEMP_txtcomment)."', '".mysql_real_escape_string($RSDB_TEMP_txtconclusion)."', '".mysql_real_escape_string($RSDB_intern_user_id)."', NOW( ) , '', '', '', '".mysql_real_escape_string($RSDB_TEMP_txtrevision)."' );";
		//echo $report_submit;
		$db_report_submit=mysql_query($report_submit);
		echo "<p><b>Your Compatibility Test Report has been saved!</b></p>";
		echo "<p>&nbsp;</p>";
		echo '<p><b><a href="'.$RSDB_intern_link_item_item2.'screens&amp;addbox=add">Submit Screenshots</a></b></p>';
		echo "<p>&nbsp;</p>";
		echo "<p><a href=\"". $RSDB_intern_link_item_item2 ."tests\">Show all compatibility test reports</a></p>";
		
		
		// Stats update:
		$update_stats_entry = "UPDATE `rsdb_stats` SET
								`stat_s_ictest` = (stat_s_ictest + 1) 
								WHERE `stat_date` = '". date("Y-m-d") ."' LIMIT 1 ;";
		mysql_query($update_stats_entry);
	}
	else {
?>

<form name="RSDB_comp_testreport" method="post" action="<?php echo $RSDB_intern_link_submit_comp_test; ?>submit">

<p><font size="2">Please write full sentenses and avoid abbreviations!</font></p>
<p><font size="2"><strong>What works:</strong><br />
    <textarea name="txtwhatwork" cols="70" rows="5" id="txtwhatwork"><?php echo $RSDB_TEMP_txtwhatwork; ?></textarea>
</font></p>
<p><font size="2"><strong>What does not work</strong>: (optional) <br />
      <textarea name="txtwhatnot" cols="70" rows="5" id="txtwhatnot"><?php echo $RSDB_TEMP_txtwhatnot; ?></textarea>
</font></p>
<p><font size="2"><strong>Describe what you have tested and what not:</strong><br />
      <textarea name="txtwhatnottested" cols="70" rows="5" id="txtwhatnottested"><?php echo $RSDB_TEMP_txtwhatnottested; ?></textarea>
</font></p>
<p><font size="2"><strong>Tester Comment:</strong> (optional) <br />
      <textarea name="txtcomment" cols="70" rows="5" id="txtcomment"><?php echo $RSDB_TEMP_txtcomment; ?></textarea>
</font></p>
<p><font size="2"><strong>Application additional information: </strong> (optional)<br>
    <input name="txtrevision" type="text" id="txtrevision" value="<?php echo $RSDB_TEMP_txtrevision; ?>" size="15" maxlength="15">
    <em><font size="1">(language, misc versions string; e.g. &quot;3.4 German&quot;, &quot;Demo&quot;, etc.)</font></em></font></p>
<p>&nbsp;</p>
<p><strong><font size="2">Application function:</font></strong></p>
<ul>
  <li><font size="2">
    <input name="optfunc" type="radio" value="5" <?php if ($RSDB_TEMP_optfunc == 5) echo "checked"; ?>> 
    5 stars = works fantastic</font></li>
  <li><font size="2">
    <input type="radio" name="optfunc" value="4" <?php if ($RSDB_TEMP_optfunc == 4) echo "checked"; ?>>
     4 stars = works good, minor bugs </font></li>
  <li><font size="2">
    <input name="optfunc" type="radio" value="3" <?php if ($RSDB_TEMP_optfunc == 3) echo "checked"; ?>>
    3 stars = works with bugs </font></li>
  <li><font size="2">
    <input type="radio" name="optfunc" value="2" <?php if ($RSDB_TEMP_optfunc == 2) echo "checked"; ?>>
    2 stars = major bugs</font></li>
  <li><font size="2">
    <input name="optfunc" type="radio" value="1" <?php if ($RSDB_TEMP_optfunc == 1) echo "checked"; ?>> 
    1 star = doesn't work, or crash while start phase </font></li>
</ul>
<p><strong><font size="2">Installation routine:</font></strong></p>
<ul>
  <li><font size="2">
    <input type="radio" name="optinstall" value="5" <?php if ($RSDB_TEMP_optinstall == 5) echo "checked"; ?>>
5 stars = works fantastic or no install routine </font></li>
  <li><font size="2">
  <input type="radio" name="optinstall" value="4" <?php if ($RSDB_TEMP_optinstall == 4) echo "checked"; ?>>
4 stars = works good, minor bugs </font></li>
  <li><font size="2">
  <input name="optinstall" type="radio" value="3" <?php if ($RSDB_TEMP_optinstall == 3) echo "checked"; ?>>
3 stars =  works with bugs </font></li>
  <li><font size="2">
  <input type="radio" name="optinstall" value="2" <?php if ($RSDB_TEMP_optinstall == 2) echo "checked"; ?>>
2 stars =  major bugs</font></li>
  <li><font size="2">
  <input name="optinstall" type="radio" value="1" <?php if ($RSDB_TEMP_optinstall == 1) echo "checked"; ?>>
1 star = doesn't work, or crash while start phase </font></li>
  </ul>
<p>&nbsp;</p>
<p><font size="2"><strong>Conclusion:</strong><br />
    <textarea name="txtconclusion" cols="70" rows="5" id="txtconclusion"><?php echo $RSDB_TEMP_txtconclusion; ?></textarea>
</font></p>
<p><font size="2">
  <input name="submitpost" type="hidden" id="submitpost" value="yes">
  Everyone will be able to vote on your compatibility test.</font></p>
					<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">By clicking &quot;Submit&quot; below you agree to be bound by the <a href="<?php echo $RSDB_intern_index_php; ?>?page=conditions" target="_blank">submit conditions</a>.</font></p>
<p>
  <input type="submit" name="Submit" value="Submit">
</p>
</form>
<?php
	}
	
}
?>