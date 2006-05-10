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
?>
<div class="contentSmall"> <span class="contentSmallTitle">Admin Interface - Page Generator</span>
<p>The static page generator can output the pages to files or to the browser.</p>
    <table border="0" cellspacing="0" cellpadding="0" width="650">
    <tr> 
      <td colspan="3"><span class="contentSmallTitle">Page Generator Overview 
        </span></td>
    </tr>
    <tr> 
      <td colspan="2" bgcolor="#F9F8F8"> <table width="650" border="0" cellpadding="4">
          <tr>
            <td><div align="center"><img src="images/dot.gif" vspace="3"></div></td>
            <td><strong><font face="Arial, Helvetica, sans-serif"><a href="?page=admin&amp;sec=generator&amp;sec2=output&amp;newcontent=true" title="Generate all UPDATED static pages! (except content that is visible on several pages, e.g. menu bars -> then use the 'generate all pages' link instead!)">Generate  all UPDATED static pages</a> </font></strong></td>
            <td>&nbsp;</td>
            <td>
              <div align="center"><img src="images/dot.gif" vspace="3"></div></td>
            <td width="300">
              <div align="left"><strong><font face="Arial, Helvetica, sans-serif"><a href="?page=admin&amp;sec=generator&amp;sec2=view&amp;site=index&amp;lang=en&amp;forma=html&amp;skin=default&amp;debug=yes" title="Don't use this function if you don't know what this function do!">View page - debug mode</a></font></strong></div></td>
          </tr>
          <tr>
            <td><div align="center"><img src="images/dot.gif" vspace="3"></div></td>
            <td><strong><font face="Arial, Helvetica, sans-serif"><a href="?page=admin&amp;sec=generator&amp;sec2=output" title="Generate all static pages (only if you need to update all pages, e.g. one time per day)">Generate all static pages</a> </font></strong></td>
            <td>&nbsp;</td>
            <td><div align="center"><img src="images/dot.gif" vspace="3"></div></td>
            <td><strong><font face="Arial, Helvetica, sans-serif"><a href="?page=admin&amp;sec=generator&amp;sec2=view&amp;site=&amp;lang=en&amp;forma=xhtml&amp;skin=default" title="Don't use this function if you don't know what this function do!">View all pages - test mode</a></font></strong></td>
          </tr>
          <tr> 
            <td width="20"> <div align="center"><img src="images/dot.gif" vspace="3"></div></td>
            <td width="300"> <div align="left"><strong><font face="Arial, Helvetica, sans-serif"><a href="#gensinglepage" title="Generate one static page (if you want to update one specific page)">Generate/view a specific static page</a></font></strong></div></td>
            <td width="10">&nbsp;</td>
            <td>
              <div align="center"><img src="images/dot.gif" vspace="3"></div></td>
            <td>
              <div align="left"><strong><font face="Arial, Helvetica, sans-serif"><a href="?page=admin&amp;sec=generator&amp;sec2=view&amp;site=&amp;lang=en&amp;forma=xhtml&amp;skin=default&amp;debug=yes" title="Don't use this function if you don't know what this function do!">View all pages - debug mode</a></font></strong></div></td>
          </tr>
          <tr> 
            <td width="20"> <div align="center"><img src="images/dot.gif" vspace="3"></div></td>
            <td width="300"> <div align="left"><strong><font face="Arial, Helvetica, sans-serif"><a href="?page=admin&amp;sec=generator&amp;sec2=view&amp;sec3=menu&amp;site=index&amp;lang=en&amp;forma=html&amp;skin=default" title="View the static homepage in a dynamic way">View page (dynamic from database)</a></font></strong></div></td>
            <td width="10">&nbsp;</td>
            <td>&nbsp;</td>
            <td>&nbsp;</td>
          </tr>
        </table></tr>
    <tr bgcolor=#AEADAD> 
      <td><img src="images/line.gif" width="1" height="1"></td>
    </tr>
  </table>
  <p>&nbsp; </p>
  <span class="contentSmallTitle"><a name="gensinglepage"></a>Generate one static 
  page</span> 
  <p>Generate one static page; e.g. if you want to update one specific page</p>
  <?php
	if($roscms_intern_account_level>50) {
		$rpm_page_active="";
		$rpm_page_active_set="";
		if (array_key_exists("page_active", $_GET)) $rpm_page_active=$_GET["page_active"];
		if (array_key_exists("page_active_set", $_GET)) $rpm_page_active_set=$_GET["page_active_set"];

		if ($rpm_page_active != "" AND $rpm_page_active_set != "") {
			$page_postc="UPDATE `pages` SET `page_active` = '$rpm_page_active' WHERE `page_id` = '$rpm_page_active_set' LIMIT 1 ;";
			$page_post_listc=mysql_query($page_postc);
		}
	}


	if ($rpm_lang_id == "") {
		$rpm_lang_id="all";
	}
	echo '<p>Language: ';
	if ($rpm_lang_id == "all") {	
		echo '<b>All</b>';
		$ros_cms_intern_pages_lang = "";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid=all#gensinglepage">All</a>';
	}
	echo ' | ';
	if ($rpm_lang_id == "nolang") {	
		echo '<b>International</b>';
		$ros_cms_intern_pages_lang = "AND page_language = 'all'";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid=nolang#gensinglepage">International</a>';
	}
	// Languages
	$sql_lang="SELECT * 
				FROM languages
				WHERE lang_level != '0'
				ORDER BY 'lang_level' DESC";
	$sql_query_lang=mysql_query($sql_lang);
	while($myrow_lang=mysql_fetch_row($sql_query_lang)) {
		$roscms_sel_lang = $myrow_lang[0];
		echo ' | ';
		if ($rpm_lang_id == $roscms_sel_lang) {	
			echo '<b>'.$myrow_lang[1].'</b>';
			$ros_cms_intern_pages_lang = "AND page_language = '".$roscms_sel_lang."'";
		}
		else {
			echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$roscms_sel_lang.'#gensinglepage">'.$myrow_lang[1].'</a>';
		}
	}


	if ($rpm_filt == "") {
		$rpm_filt="active";
	}
	echo '<p>Filter: ';
	if ($rpm_filt == "active") {	
		echo '<b>all (active) pages</b>';
		$ros_cms_intern_pages_filt = "WHERE page_active = '1'";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt=active&amp;langid='.$rpm_lang_id.'#gensinglepage">all (active) pages</a>';
	}
	echo ' | ';
	if ($rpm_filt == "user") {	
		echo '<b>current user</b>';
		$ros_cms_intern_pages_filt = "WHERE page_active = '1' AND page_usrname_id = '".$roscms_intern_account_id."'";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt=user&amp;langid='.$rpm_lang_id.'#gensinglepage">current user</a>';
	}
	echo ' | ';
	if ($rpm_filt == "normalpages") {	
		echo '<b>normal pages</b>';
		$ros_cms_intern_pages_filt = "WHERE page_active = '1' AND pages_extra = ''";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt=normalpages&amp;langid='.$rpm_lang_id.'#gensinglepage">normal pages</a>';
	}
	echo ' | ';
	if ($rpm_filt == "dynamicpages") {	
		echo '<b>dynamic pages</b>';
		$ros_cms_intern_pages_filt = "WHERE page_active = '1' AND pages_extra != ''";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort='.$rpm_sort.'&amp;filt=dynamicpages&amp;langid='.$rpm_lang_id.'#gensinglepage">dynamic pages</a>';
	}
	echo '</p>';

	if ($rpm_sort == "") {
		$rpm_sort="id";
	}
	echo '<p>Sorted by: ';
	if ($rpm_sort == "id") {	
		echo '<b>page ID</b>';
		$ros_cms_intern_pages_sortby="page_name";
		$ros_cms_intern_pages_sort="ASC";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort=id&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'#gensinglepage">page ID</a>';
	}
	echo ' | ';
	if ($rpm_sort == "date") {	
		echo '<b>date</b>';
		$ros_cms_intern_pages_sortby="page_date";
		$ros_cms_intern_pages_sort="DESC";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort=date&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'#gensinglepage">date</a>';
	}
	echo ' | ';
	if ($rpm_sort == "user") {	
		echo '<b>user</b>';
		$ros_cms_intern_pages_sortby="page_usrname_id";
		$ros_cms_intern_pages_sort="ASC";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort=user&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'#gensinglepage">user</a>';
	}
	echo ' | ';
	if ($rpm_sort == "active") {	
		echo '<b>active</b>';
		$ros_cms_intern_pages_sortby="page_active";
		$ros_cms_intern_pages_sort="DESC";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort=active&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'#gensinglepage">active</a>';
	}
	echo ' | ';
	if ($rpm_sort == "visible") {	
		echo '<b>visible</b>';
		$ros_cms_intern_pages_sortby="page_visible";
		$ros_cms_intern_pages_sort="DESC";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort=visible&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'#gensinglepage">visible</a>';
	}
	echo ' | ';
	if ($rpm_sort == "version") {	
		echo '<b>version</b>';
		$ros_cms_intern_pages_sortby="page_version";
		$ros_cms_intern_pages_sort="DESC";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort=version&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'#gensinglepage">version</a>';
	}
	echo ' | ';
	if ($rpm_sort == "extra") {	
		echo '<b>extra</b>';
		$ros_cms_intern_pages_sortby="pages_extra";
		$ros_cms_intern_pages_sort="ASC";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort=extra&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'#gensinglepage">extra</a>';
	}
	echo ' | ';
	if ($rpm_sort == "language") {	
		echo '<b>language</b>';
		$ros_cms_intern_pages_sortby="page_language";
		$ros_cms_intern_pages_sort="ASC";
	}
	else {
		echo '<a href="?page=admin&amp;sec=generate&amp;sec2=view&amp;sort=language&amp;filt='.$rpm_filt.'&amp;opt='.$rpm_opt.'&amp;langid='.$rpm_lang_id.'#gensinglepage">language</a>';
	}
	echo '</p>';

?>
  <table width="100%" border="0" cellpadding="1" cellspacing="1">
    <tr bgcolor="#5984C3"> 
      <td width="9%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Action</strong></font></div></td>
      <td width="8%" colspan="3" bgcolor="#5984C3"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Info</strong></font></div></td>
      <td width="13%" bgcolor="#5984C3"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Page 
          ID</strong></font></div></td>
      <td width="13%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Language</strong></font></div></td>
      <td width="13%" bgcolor="#5984C3"> 
        <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Latest 
          Gen.</strong></font></div></td>
      <td width="7%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Rev.</strong></font></div>
        <div align="center"></div></td>
      <td width="13%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong> 
          Change</strong></font></div></td>
      <td width="23%"> 
        <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>User 
          (Generator) </strong></font></div></td>
    </tr>
    <?php

	if($roscms_intern_account_level==100) {
		$query_page = mysql_query("SELECT * 
				FROM pages
				$ros_cms_intern_pages_filt $ros_cms_intern_pages_lang
				ORDER BY '$ros_cms_intern_pages_sortby' $ros_cms_intern_pages_sort") ;
	}
	else {
		$query_page = mysql_query("SELECT * 
				FROM pages
				$ros_cms_intern_pages_filt AND page_visible != 0 $ros_cms_intern_pages_lang
				ORDER BY '$ros_cms_intern_pages_sortby' $ros_cms_intern_pages_sort") ;
	}

	$farbe1=$roscms_intern_color1;
	$farbe2=$roscms_intern_color2;
	$zaehler="0";
	//$farbe="#CCCCC";
	
	while($result_page = mysql_fetch_array($query_page)) { // Pages
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
								if ($result_page['page_generate_force'] == "1") {
									$farbe = "#00FF00";
								}
							 ?>" title="RosCMS action buttons:&#10;&#10;* Generate static page&#10;* View page in dynamc mode&#10;* Delete static pagse"> 
        <div align="center">
          <?php 
		  	$cmsros_intern_temp_lang_short=$result_page['page_language'];
			$sql_lang="SELECT * 
				FROM languages
				WHERE lang_level != '0' AND lang_id = '$cmsros_intern_temp_lang_short'
				ORDER BY 'lang_level' DESC";
			$sql_query_lang=mysql_query($sql_lang);
			$myrow_lang=mysql_fetch_row($sql_query_lang); // Languages
		?><a href="?page=admin&amp;sec=generator&amp;sec2=genpage&amp;site=<?php echo $result_page['page_name'];?>&amp;lang=<?php echo $myrow_lang[0]; ?>&amp;skin=default" target="_blank"><img src="images/genpage.gif" alt="Generate this page (xhtml and html)" width="19" height="18" border="0"></a> 
          <a href="?page=admin&amp;sec=generator&amp;sec2=view&amp;site=<?php echo $result_page['page_name'];?>&amp;lang=<?php echo $myrow_lang[0]; ?>&amp;forma=xhtml&amp;skin=default" target="_blank"><img src="images/view.gif" alt="View page dynamic" width="19" height="18" border="0"></a> 
          <?php
			 if($roscms_intern_account_level==100) { ?>
          <script type="text/javascript">
			<!--
				function DeletePage() {
					var chk = window.confirm("Do you really want to delete the static xhtml/html pages?");
					if (chk == true) {
						//parent.location.href = "?page=admin&amp;sec=generate&amp;sec2=delete&amp;db_id=<?php echo $result_page['page_id']; ?>";
						alert("This function will be finished soon!");
					}
				}
			-->
			</script><a href="javascript:DeletePage()"><img src="images/delete.gif" alt="Delete the static pages (xhtml/html) and NOT the page content in the database!" width="19" height="18" border="0"></a><?php  } ?>        </div></td>
      <td width="3%" valign="middle" bgcolor="<?php echo $farbe; ?>"><div align="center"> 
          <?php
		 if($result_page['page_active'] == "1") { ?>
          <img src="images/active.gif" alt="active" width="19" height="18" border="0">
          <?php
		 }
		 else { ?>
          <img src="images/notactive.gif" alt="NOT active" width="19" height="18" border="0">
          <?php } ?>
        </div></td>
      <td width="3%" valign="middle" bgcolor="<?php echo $farbe; ?>"><div align="center"> 
          <?php
		 if($result_page['page_visible'] == "1") { ?>
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
          <?php } ?>
        </div></td>
      <td width="13%" valign="middle" bgcolor="<?php echo $farbe; ?>"> <div align="left"><font face="Arial, Helvetica, sans-serif"><?php echo "<b>".$result_page['page_name']."</b>"; ?></font></div></td>
      <td width="13%" valign="middle" bgcolor="<?php if ($result_page['page_language'] != "") { echo $farbe; } else { echo "#FF0000"; } ?>"><font face="Arial, Helvetica, sans-serif"> 
        <div align="center"> 
          <?php 
			if ($myrow_lang[1] != "") {
				echo $myrow_lang[1];
			}
			else if ($result_page['page_language'] != "all") {
				echo $result_page['page_language'];
			}
		?>
        </div>
        </font></td>
      <td width="13%" valign="middle" bgcolor="<?php echo $farbe; ?>"> <div align="center"><font face="Arial, Helvetica, sans-serif">
          <?php 
			echo "<b>".date("Y-m-d",$result_page['page_generate_timestamp'])."</b><br>".date("H:i:s",$result_page['page_generate_timestamp']);
		?>
          </font></div></td>
      <td width="7%" valign="middle" bgcolor="<?php echo $farbe; ?>"> <div align="right"><font face="Arial, Helvetica, sans-serif"> 
          <?php 
			echo $result_page['page_version'];
		?>
          </font></div>
        <div align="center"><font face="Arial, Helvetica, sans-serif"> </font></div></td>
      <td width="13%" valign="middle" bgcolor="<?php echo $farbe; ?>"> <div align="center"><font face="Arial, Helvetica, sans-serif"> 
          <?php 
			echo $result_page['page_date']." ".$result_page['page_time'];;
		?>
          </font></div></td>
      <td width="23%" valign="middle" bgcolor="<?php echo $farbe; ?>"> 
        <div align="center"><font face="Arial, Helvetica, sans-serif"> 
          <?php 
			$accountinfo_query = @mysql_query("SELECT user_name, user_id FROM users WHERE user_id = '".$result_page['page_generate_usrid']."'") or die('DB error (admin interface)!');
			$accountinfo_result = @mysql_fetch_array($accountinfo_query);

			echo "<b>".$accountinfo_result['user_name']."</b><br><a href='?page=user&amp;sec=profil&amp;sec2=".$result_page['page_usrname_id']."' target='_blank'>Profile</a>";
			
		?> </font></div></td>
    </tr>
    <?php	
	}	// end while
?>
  </table>
    <?php
	include("inc/inc_description_table.php");
?>
<p>&nbsp;</p>
</div>
