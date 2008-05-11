<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Klemens Friedl <frik85@reactos.org>

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

	// To prevent hacking activity:
	if ( !defined('ROSCMS_SYSTEM') )
	{
		if ( !defined('ROSCMS_SYSTEM_LOG') ) {
			define ("ROSCMS_SYSTEM_LOG", "Hacking attempt");
		}
		$seclog_section="roscms_interface";
		$seclog_level="50";
		$seclog_reason="Hacking attempt: admin_content.php";
		define ("ROSCMS_SYSTEM", "Hacking attempt");
		include('securitylog.php'); // open security log
		die("Hacking attempt");
	}
	if ($rpm_page != "admin" && $rpm_page != "dev" && $rpm_page != "team" && $rpm_page != "trans") {
		die("");
	}
	if ($roscms_intern_usrgrp_admin == true || $roscms_intern_usrgrp_dev == true || $roscms_intern_usrgrp_team == true || $roscms_intern_usrgrp_trans == true) {

?>
 
<div class="contentSmall"> <span class="contentSmallTitle"><?php
	if ($rpm_page == "admin") {
		echo "Admin Interface - Content";
	}
	elseif ($rpm_page == "dev") {
		echo "Dev Interface - Content";
	}
	elseif ($rpm_page == "trans") {
		echo "Translator Interface - Content";
	}
	elseif ($rpm_page == "team") {
		echo "Team Interface - Content";
	}
	else {
		echo $rpm_page." Interface - Content";
	}
  ?></span> 
  <ul>
    <li><strong>Content</strong></li>
  </ul>
  <p>Action: <?php if ($roscms_intern_usrgrp_admin == true && $rpm_page == "admin") { ?><a href="?page=<?php echo $rpm_page; ?>&amp;sec=content&amp;sec2=edit&amp;opt=insert&amp;<?php echo 'sort='.$rpm_sort.'&amp;filt='.$rpm_filt.'&amp;langid='.$rpm_lang_id ; ?>&amp;db_id=new">New 
    Content</a> | <?php } ?><a href="?page=<?php echo $rpm_page; ?>&sec=content&sec2=view">reset filters & 
    sort</a></p>
    
  <?php
	if($roscms_intern_usrgrp_admin == true) {
		$rpm_content_active="";
		$rpm_content_active_set="";
		if(array_key_exists("content_active", $_GET)) $rpm_content_active=$_GET["content_active"];
		if(array_key_exists("content_active_set", $_GET)) $rpm_content_active_set=$_GET["content_active_set"];
	
		if ($rpm_content_active != "" AND $rpm_content_active_set != "") {
			$content_postc="UPDATE `content` SET `content_active` = '$rpm_content_active' WHERE `content_id` = '". $rpm_content_active_set ."' LIMIT 1 ;";
			$content_post_listc=mysql_query($content_postc);
		}
	}


	if ($rpm_lang_id == "") {
		if (($roscms_intern_usrgrp_trans == true || $roscms_intern_usrgrp_team == true) && ($rpm_page == "trans" || $rpm_page == "team")) {
			$rpm_lang_id="nolang";
		}
		else {
			$rpm_lang_id="all";
		}
	}
	echo '<p>Language: ';
	if ($roscms_intern_usrgrp_dev == true || $roscms_intern_usrgrp_admin == true) {
		if ($rpm_lang_id == "all") {	
			echo '<b>All</b>';
			$ros_cms_intern_content_lang = "";
		}
		else {
			echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid=all">All</a>';
		}
		echo ' | ';
	}

		if ($rpm_lang_id == "nolang") {	
			echo '<b>International</b>';
			if ($roscms_intern_usrgrp_admin == true) {
				$ros_cms_intern_content_lang = "AND content_lang = 'all'";
			}
			if ($roscms_intern_usrgrp_dev == true && $rpm_page == "dev") {
				$ros_cms_intern_content_lang = "AND content_lang = 'all'";
			}
			if (($roscms_intern_usrgrp_team == true && $rpm_page == "team") || ($roscms_intern_usrgrp_trans == true && $rpm_page == "trans")) {
				$ros_cms_intern_content_lang = "AND content_lang = 'all' AND content_type = 'default'";
			}
		}
		else {
			echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid=nolang">International</a>';
		}
	// Languages
	$sql_lang="SELECT * 
				FROM languages
				WHERE lang_level != '0'
				ORDER BY 'lang_level' DESC";
	$sql_query_lang=mysql_query($sql_lang);
	while($myrow_lang=mysql_fetch_row($sql_query_lang)) {
		if ($roscms_intern_usrgrp_dev != true || $roscms_intern_usrgrp_admin != true) {
			if ($myrow_lang[0] == "en") {
				continue;
			}
		}
		$roscms_sel_lang = $myrow_lang[0];
		echo ' | ';
		if ($rpm_lang_id == $roscms_sel_lang) {	
			echo '<b>'.$myrow_lang[1].'</b>';
			$ros_cms_intern_content_lang = "AND content_lang = '".$roscms_sel_lang."'";
		}
		else {
			echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$roscms_sel_lang.'">'.$myrow_lang[1].'</a>';
		}
	}


	if ($rpm_filt == "") {
		if ($roscms_intern_usrgrp_admin == true) {
			$rpm_filt="active";
		}
		if ($roscms_intern_usrgrp_dev == true && $rpm_page == "dev") {
			$rpm_filt="anvc";
		}
		if (($roscms_intern_usrgrp_team == true && $rpm_page == "team") || ($roscms_intern_usrgrp_trans == true && $rpm_page == "trans")) {
			$rpm_filt="anvc";
		}
	}
	echo '<p>Filter: ';
	if (($roscms_intern_usrgrp_admin == true && $rpm_page == "admin") || ($roscms_intern_usrgrp_dev == true && $rpm_page == "dev")) {
		if ($rpm_filt == "active") {	
			echo '<b>active content</b>';
			$ros_cms_intern_content_filt = "WHERE content_active = '1'";
		}
		else {
			echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt=active&amp;langid='.$rpm_lang_id.'">active content</a>';
		}
		echo ' | ';
		if ($rpm_filt == "all") {	
			echo '<b>all content</b>';
			$ros_cms_intern_content_filt = "WHERE `content_name` != ''";
		}
		else {
			echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt=all&amp;langid='.$rpm_lang_id.'">all content</a>';
		}
		echo ' | ';
	}
	if ($rpm_filt == "anvc") {	
		echo '<b>active and visible content</b>';
		$ros_cms_intern_content_filt = "WHERE content_active = '1' AND content_visible = '1'";
	}
	else {
		echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt=anvc&amp;langid='.$rpm_lang_id.'">active and visible content</a>';
	}
	echo ' | ';
	if ($rpm_filt == "user") {	
		echo '<b>current user</b>';
		$ros_cms_intern_content_filt = "WHERE content_usrname_id = '".$roscms_intern_account_id."'";
	}
	else {
		echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt=user&amp;langid='.$rpm_lang_id.'">current user</a>';
	}
	if ($rpm_filt == "history") {	
		echo ' | <b>history</b>';
		$ros_cms_intern_content_filt = "WHERE content_name = '".$rpm_opt."'";
	}
	echo '</p>';

	if ($rpm_sort == "") {
		$rpm_sort="id";
	}
	echo '<p>Sorted by: ';
	if ($rpm_sort == "id") {	
		echo '<b>content id</b>';
		$ros_cms_intern_content_sortby="content_name";
		$ros_cms_intern_content_sort="ASC";
	}
	else {
		echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort=id&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'">content id</a>';
	}
	echo ' | ';
	if ($rpm_sort == "date") {	
		echo '<b>date</b>';
		$ros_cms_intern_content_sortby="content_date";
		$ros_cms_intern_content_sort="DESC";
	}
	else {
		echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort=date&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'">date</a>';
	}
	echo ' | ';
	if ($rpm_sort == "user") {	
		echo '<b>user</b>';
		$ros_cms_intern_content_sortby="content_usrname_id";
		$ros_cms_intern_content_sort="ASC";
	}
	else {
		echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort=user&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'">user</a>';
	}
	echo ' | ';
	if ($rpm_sort == "active") {	
		echo '<b>active</b>';
		$ros_cms_intern_content_sortby="content_active";
		$ros_cms_intern_content_sort="DESC";
	}
	else {
		echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort=active&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'">active</a>';
	}
	echo ' | ';
	if ($rpm_sort == "visible") {	
		echo '<b>visible</b>';
		$ros_cms_intern_content_sortby="content_visible";
		$ros_cms_intern_content_sort="DESC";
	}
	else {
		echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort=visible&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'">visible</a>';
	}
	echo ' | ';
	if ($rpm_sort == "version") {	
		echo '<b>version</b>';
		$ros_cms_intern_content_sortby="content_version";
		$ros_cms_intern_content_sort="DESC";
	}
	else {
		echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort=version&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'">version</a>';
	}
	echo ' | ';
	if ($rpm_sort == "language") {	
		echo '<b>language</b>';
		$ros_cms_intern_content_sortby="content_lang";
		$ros_cms_intern_content_sort="ASC";
	}
	else {
		echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort=language&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'">language</a>';
	}
	echo ' | ';
	if ($rpm_sort == "editor") {	
		echo '<b>editor</b>';
		$ros_cms_intern_content_sortby="content_editor";
		$ros_cms_intern_content_sort="DESC";
	}
	else {
		echo '<a href="?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort=editor&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'">editor</a>';
	}
	echo '</p>';

?>
  <table width="100%" border="0" cellpadding="1" cellspacing="1">
    <tr bgcolor="#5984C3"> 
      <td width="9%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Action</strong></font></div></td>
      <td width="8%" colspan="3" bgcolor="#5984C3"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Info</strong></font></div></td>
      <td width="13%" bgcolor="#5984C3"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Content 
          ID</strong></font></div></td>
      <td width="13%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Language</strong></font></div></td>
      <td width="26%" bgcolor="#5984C3"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Content</strong></font></div></td>
      <td width="7%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Rev.</strong></font></div>
        <div align="center"></div></td>
      <td width="13%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong> 
          Date</strong></font></div></td>
      <td width="10%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>User</strong></font></div></td>
    </tr>
    <?php

	if($roscms_intern_usrgrp_sadmin == true) {
		$query_content = mysql_query("SELECT * 
				FROM content
				$ros_cms_intern_content_filt $ros_cms_intern_content_lang
				ORDER BY '$ros_cms_intern_content_sortby' $ros_cms_intern_content_sort") ;
		/*$query_content = mysql_query("SELECT * 
				FROM content
				$ros_cms_intern_content_filt $ros_cms_intern_content_lang
				ORDER BY '$ros_cms_intern_content_sortby' $ros_cms_intern_content_sort") ;*/
	}
	elseif ($roscms_intern_usrgrp_admin == true) {
		$query_content = mysql_query("SELECT * 
				FROM content
				$ros_cms_intern_content_filt AND content_visible != 0 $ros_cms_intern_content_lang
				ORDER BY '$ros_cms_intern_content_sortby' $ros_cms_intern_content_sort") ;
	}
	elseif ($roscms_intern_usrgrp_dev == true) {
		$query_content = mysql_query("SELECT * 
				FROM content
				$ros_cms_intern_content_filt AND content_visible != 0 $ros_cms_intern_content_lang
				ORDER BY '$ros_cms_intern_content_sortby' $ros_cms_intern_content_sort") ;
	}
	elseif ($roscms_intern_usrgrp_team == true || $roscms_intern_usrgrp_trans == true) {
		$query_content = mysql_query("SELECT * 
				FROM content
				$ros_cms_intern_content_filt AND content_visible != 0 $ros_cms_intern_content_lang
				ORDER BY '$ros_cms_intern_content_sortby' $ros_cms_intern_content_sort") ;
	}
	else {
		die("");
	}

	$farbe1=$roscms_intern_color1;
	$farbe2=$roscms_intern_color2;
	$zaehler="0";
	//$farbe="#CCCCC";
	
	while($result_content = mysql_fetch_array($query_content)) { // content
?>
    <tr> 
      <td width="9%" valign="middle" bgcolor="<?php
								$zaehler++;
								if ($zaehler == "1") {
									echo $farbe1;
									$farbe = $farbe1;
								}
								elseif ($zaehler == "2") {
									$zaehler="0";
									echo $farbe2;
									$farbe = $farbe2;
								}
							 ?>"> 
        <div align="center"> 
          <a name="<?php echo $result_content['content_id']; ?>"></a>
		  <?php
		  if (($roscms_intern_usrgrp_trans == true || $roscms_intern_usrgrp_team == true) && ($rpm_page == "trans" || $rpm_page == "team") && $rpm_lang_id == "nolang") { ?>
          <a href="?page=<?php echo $rpm_page; ?>&amp;sec=content&amp;sec2=edit&amp;opt=translate&amp;<?php echo 'sort='.$rpm_sort.'&amp;filt='.$rpm_filt.'&amp;langid='.$rpm_lang_id.'&amp;db_id='.$result_content['content_id']; ?>"><img src="images/tool.gif" alt="Translate" width="19" height="18" border="0"></a> 
          <?php } else { ?>
          <a href="?page=<?php echo $rpm_page; ?>&amp;sec=content&amp;sec2=edit&amp;<?php echo 'sort='.$rpm_sort.'&amp;filt='.$rpm_filt.'&amp;langid='.$rpm_lang_id.'&amp;db_id='.$result_content['content_id']; ?>"><img src="images/view.gif" alt="View" width="19" height="18" border="0"></a> 
          <?php
		  }
		  if($roscms_intern_usrgrp_sadmin == true) { ?>
          <script type="text/javascript">
			<!--
				function DeleteContent() {
					var chk = window.confirm("Do you really want to delete this content?");
					if (chk == true) {
						//parent.location.href = "?page=admin&amp;sec=content&amp;sec2=delete&amp;db_id=<?php echo $result_content['content_id']; ?>";
						alert("Sorry! This feature has been disabled. Please ask the administrator if you really want to delete a content.");
					}
				}
			-->
			</script>
          <a href="javascript:DeleteContent()"><img src="images/delete.gif" alt="Delete" width="19" height="18" border="0"></a> 
          <?php } 
		  if($roscms_intern_usrgrp_admin == true || $roscms_intern_usrgrp_team == true) {?>
			  <a href="<?php echo "?page=".$rpm_page."&amp;sec=content&amp;sec2=view&amp;sort=version&amp;filt=history&amp;opt=".$result_content['content_name']."&amp;langid=".$result_content['content_lang']; ?>"><img src="images/history.gif" alt="Filter: history" width="19" height="18" border="0"></a> 
          <?php } ?>
        </div></td>
      <td width="3%" valign="middle" bgcolor="<?php echo $farbe; ?>"><div align="center"> 
          <?php
		 if($result_content['content_active'] == "1") { ?>
          <a href="<?php if($roscms_intern_usrgrp_admin == true) { echo '?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'&amp;content_active=0&amp;content_active_set='.$result_content['content_id'] ; } else { echo '#'; } ?>"><img src="images/active.gif" alt="active" width="19" height="18" border="0"></a> 
          <?php
		 }
		 else { ?>
          <a href="<?php if($roscms_intern_usrgrp_admin == true) { echo '?page='.$rpm_page.'&amp;sec=content&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'&amp;content_active=1&amp;content_active_set='.$result_content['content_id'] ; } else { echo '#'; } ?>"><img src="images/notactive.gif" alt="NOT active" width="19" height="18" border="0"></a> 
          <?php } ?>
        </div></td>
      <td width="3%" valign="middle" bgcolor="<?php echo $farbe; ?>"><div align="center"> 
          <?php
		 if($result_content['content_visible'] == "1") { ?>
          <img src="images/visible.gif" alt="visible" width="19" height="18" border="0"> 
          <?php
		 }
		 else { ?>
          <img src="images/notvisible.gif" alt="NOT visible" width="19" height="18" border="0"> 
          <?php } ?>
        </div></td>
      <td width="3%" valign="middle" bgcolor="<?php echo $farbe; ?>"><div align="center"> 
          <?php if($roscms_intern_account_level<=10) { ?>
          <img src="images/lock.gif" alt="Locked" width="19" height="18"> 
          <?php } else if ($result_content['content_editor'] == "richtext") { ?>
			  <img src="images/richtexteditor.gif" alt="Rich Text Editor" width="19" height="18"> 
		  <?php } else if ($result_content['content_editor'] == "bbcode") { ?>
			  <img src="images/bbcode.gif" alt="bbcode Editor" width="19" height="18"> 
		  <?php } ?>
        </div></td>
      <td width="13%" valign="middle" bgcolor="<?php echo $farbe; ?>"> <div align="left"><font face="Arial, Helvetica, sans-serif"><?php echo "[#cont_<b>".$result_content['content_name']."</b>]"; ?></font></div></td>
      <td width="13%" valign="middle" bgcolor="<?php if ($result_content['content_lang'] != "") { echo $farbe; } else { echo "#FF0000"; } ?>"> <div align="center"> 
          <font face="Arial, Helvetica, sans-serif">
          <?php 
		  	$cmsros_intern_temp_lang_short=$result_content['content_lang'];
			$sql_lang="SELECT * 
				FROM languages
				WHERE lang_level != '0' AND lang_id = '$cmsros_intern_temp_lang_short'
				ORDER BY 'lang_level' DESC";
			$sql_query_lang=mysql_query($sql_lang);
			$myrow_lang=mysql_fetch_row($sql_query_lang); // Languages
			if ($myrow_lang[1] != "") {
				echo $myrow_lang[1];
			}
			else if ($result_content['content_lang'] != "all") {
				echo $result_content['content_lang'];
			}
		?>
          </font></div></td>
      <td width="26%" valign="middle" bgcolor="<?php echo $farbe; ?>" title="<?php 
			if ($result_content['content_visible'] != 1) { echo "NOT visible!\n\n"; }
			echo substr(htmlentities($result_content['content_text'], ENT_QUOTES), 0, 200)."...";
		?>"> <pre><font face="Arial, Helvetica, sans-serif"><?php 
			echo substr(htmlentities($result_content['content_text'], ENT_QUOTES), 0, 40)."..." ;
		?></font></pre> </td>
      <td width="7%" valign="middle" bgcolor="<?php echo $farbe; ?>"> <div align="right"><font face="Arial, Helvetica, sans-serif"> 
          <?php 
			echo $result_content['content_version'];
		?>
          </font></div>
        <div align="center"><font face="Arial, Helvetica, sans-serif"> </font></div></td>
      <td width="13%" valign="middle" bgcolor="<?php echo $farbe; ?>"> <div align="center"><font face="Arial, Helvetica, sans-serif"> 
          <?php 
			echo $result_content['content_date']." ".$result_content['content_time'];;
		?>
          </font></div></td>
      <td width="10%" valign="middle" bgcolor="<?php echo $farbe; ?>"> <div align="center"><font face="Arial, Helvetica, sans-serif"> 
          <?php 
			$accountinfo_query = @mysql_query("SELECT user_name, user_id FROM users WHERE user_id = '".$result_content['content_usrname_id']."'") or die('DB error (admin interface)!');
			$accountinfo_result = @mysql_fetch_array($accountinfo_query);
			
			$roscms_intern_accountuser = $accountinfo_result['user_name'];
			if ($roscms_intern_accountuser && $roscms_intern_accountuser != "") {
				echo "<b>".$roscms_intern_accountuser."</b><br><a href='?page=user&amp;sec=profil&amp;sec2=".$result_content['content_usrname_id']."' target='_blank'>Profile</a>";
			}
			else {
				echo "<b>RosCMS</b>";
			}
		?>
          </font></div></td>
    </tr>
    <?php	
	}	// end while
?>
  </table>
  <?php
	include("inc/inc_description_table.php");
?>
</div>
<?php
	}
?>
