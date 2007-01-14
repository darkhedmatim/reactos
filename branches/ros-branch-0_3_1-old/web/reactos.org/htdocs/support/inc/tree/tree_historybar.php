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


//	echo "Comp: ".$RSDB_intern_trigger_comp." | Pack:".$RSDB_intern_trigger_pack." | DevNet:".$RSDB_intern_trigger_devnet." | Media:".$RSDB_intern_trigger_media;
//	echo "<br>".$RSDB_intern_code_db_rsdb_categories;

	$RSDB_viewpage = true;

	if ($RSDB_SET_sec == "category") {
?>
<table width="100%" border="1" cellpadding="3" cellspacing="0" bordercolor="#5984C3">
  <tr>
    <td>&nbsp;<font size="2">Browsing &gt;&gt;</font> <a href="<?php echo $RSDB_intern_link_category_cat ?>0">Main</a>
	
	<?php
		$RSDB_TEMP_current_category = "Main";
		$RSDB_TEMP_current_category_desc = "Root directory of the tree.";
	
		if ($RSDB_SET_cat != "") {

			if ($RSDB_SET_item != "" && $RSDB_viewpage != false) {
				$query_itemid = mysql_query("SELECT * 
												FROM `rsdb_item_" . $RSDB_intern_code_view_shortname ."` 
												WHERE `" . $RSDB_intern_code_view_shortname . "_visible` = '1'
												AND `" . $RSDB_intern_code_view_shortname . "_id` = " . $RSDB_SET_item . "
												ORDER BY `" . $RSDB_intern_code_view_shortname . "_name` ASC") ;				
				$result_itempid = @mysql_fetch_array($query_itemid);			
				if ($result_itempid[$RSDB_intern_code_view_shortname.'_groupid'] == "" || $result_itempid[$RSDB_intern_code_view_shortname . '_groupid'] == "0") {
					//die("");
					//echo "die1";
					$RSDB_viewpage = false;
				}
				$RSDB_SET_group = $result_itempid[$RSDB_intern_code_view_shortname . '_groupid'];
			}
			if ($RSDB_SET_group != "" && $RSDB_viewpage != false) {
				//echo "+++++".$RSDB_SET_group;
				$query_groupid = mysql_query("SELECT * 
												FROM `rsdb_groups` 
												WHERE `grpentr_visible` = '1'
												AND `grpentr_id` = " . $RSDB_SET_group . "
												" . $RSDB_intern_code_db_rsdb_groups . "
												ORDER BY `grpentr_name` ASC") ;				
				$result_groupid = mysql_fetch_array($query_groupid);			
				if ($result_groupid['grpentr_category'] == "" || $result_groupid['grpentr_category'] == "0") {
					//die("");
					//echo "die2";
					$RSDB_viewpage = false;
				}
				if ($RSDB_viewpage != false) {
					$query_category_treehistory_groupid= mysql_query("SELECT * 
																FROM `rsdb_categories` 
																WHERE `cat_id` = " . $result_groupid['grpentr_category'] ."
																AND `cat_visible` = '1'
																" . $RSDB_intern_code_db_rsdb_categories . " ;");
					$result_category_treehistory_groupid=mysql_fetch_array($query_category_treehistory_groupid);
					
					$RSDB_TEMP_cat_path = $result_category_treehistory_groupid['cat_path'];
					$RSDB_TEMP_cat_id = $result_category_treehistory_groupid['cat_id'];
					//$RSDB_TEMP_cat_current_level = $result_category_treehistory_groupid['cat_level'];
	
					$RSDB_TEMP_cat_current_level=1;
					$RSDB_TEMP_cat_current_id = $RSDB_TEMP_cat_id;
					//$RSDB_TEMP_cat_current_counter = $result_category_treehistory_groupid['cat_level'];
					//$RSDB_TEMP_cat_treehist = $result_category_treehistory_groupid['cat_level'];
				}
			}
			elseif ($RSDB_viewpage != false) {
				//echo "hjall";
				$query_category_treehistory= mysql_query("SELECT * 
															FROM `rsdb_categories` 
															WHERE `cat_id` = " . $RSDB_SET_cat ."
															AND `cat_visible` = '1'
															" . $RSDB_intern_code_db_rsdb_categories . " ;");
				$result_category_treehistory=mysql_fetch_array($query_category_treehistory);
				
				$RSDB_TEMP_cat_path = $result_category_treehistory['cat_path'];
				$RSDB_TEMP_cat_id = $result_category_treehistory['cat_id'];

				$RSDB_TEMP_cat_current_level=1;
				$RSDB_TEMP_cat_current_id = $RSDB_TEMP_cat_id;
			}
			if ($RSDB_viewpage != false) {
	//			echo "<p>";
				$RSDB_TEMP_cat_current_id_guess = $RSDB_TEMP_cat_current_id;
				$RSDB_intern_catlevel=0;
				
				// count the levels -> current category level
				for ($guesslevel=1; ; $guesslevel++) {
	//				echo $guesslevel."#";
						$query_category_tree_guesslevel= mysql_query("SELECT * 
																	FROM `rsdb_categories` 
																	WHERE `cat_id` = " . $RSDB_TEMP_cat_current_id_guess ."
																	AND `cat_visible` = '1'
																	" . $RSDB_intern_code_db_rsdb_categories . " ;");
						$result_category_tree_guesslevel=@mysql_fetch_array($query_category_tree_guesslevel);
	//					echo $result_category_tree_guesslevel['cat_name'];
						$RSDB_TEMP_cat_current_id_guess = $result_category_tree_guesslevel['cat_path'];
						
						if (!$result_category_tree_guesslevel['cat_name']) {
	//						echo "ENDE:".($guesslevel-1);
							$RSDB_intern_catlevel = ($guesslevel-1);
							$RSDB_TEMP_cat_current_counter = $RSDB_intern_catlevel;
							break;
						}
				}
	//			echo "<p>";
				
				
				for ($i=0; $i < $RSDB_intern_catlevel; $i++) {			
	//				echo "<br>Ring0: ".$i." ";
					for ($k=1; $k < ($RSDB_intern_catlevel+1-$i); $k++) {
	//					echo $k."|";
							$query_category_tree_temp= mysql_query("SELECT * 
																		FROM `rsdb_categories` 
																		WHERE `cat_id` = " . $RSDB_TEMP_cat_current_id ."
																		AND `cat_visible` = '1'
																		" . $RSDB_intern_code_db_rsdb_categories . " ;");
							$result_category_tree_temp=mysql_fetch_array($query_category_tree_temp);
							$RSDB_TEMP_cat_current_id = $result_category_tree_temp['cat_path'];
							
	//						echo "K:".$k."|E:".($result_category_treehistory['cat_level']+1-$i);
							if ($k == $RSDB_TEMP_cat_current_counter) {
								$RSDB_TEMP_current_category = $result_category_tree_temp['cat_name'];
								$RSDB_TEMP_current_category_desc = $result_category_tree_temp['cat_description'];
								echo " <font size='2'>&gt;</font> <a href='".$RSDB_intern_link_category_cat.$result_category_tree_temp['cat_id']."'>".$result_category_tree_temp['cat_name']."</a>";
							}
					}
					$RSDB_TEMP_cat_current_counter--;
					$k=1;
					$RSDB_TEMP_cat_current_id="";
					$RSDB_TEMP_cat_current_id = $RSDB_TEMP_cat_id;
					
				}		
				//create_historybar($RSDB_TEMP_cat_path, $RSDB_TEMP_cat_id, $RSDB_TEMP_cat_current_level);
			}
			
			if ($RSDB_SET_group != "" && $RSDB_viewpage != false) {
				$query_current_group = mysql_query("SELECT * 
								FROM `rsdb_groups` 
								WHERE `grpentr_visible` = '1'
								AND `grpentr_id` = " . $RSDB_SET_group . "
								" . $RSDB_intern_code_db_rsdb_groups . "
								ORDER BY `grpentr_name` ASC") ;
				$result_current_group = mysql_fetch_array($query_current_group);
				echo " <font size='2'>&gt;</font> <a href='".$RSDB_intern_link_group.$RSDB_SET_group."'>".$result_current_group['grpentr_name']."</a>";
			}
			if ($RSDB_SET_item != "" && $RSDB_viewpage != false) {
				$query_current_group = mysql_query("SELECT * 
								FROM `rsdb_item_" . $RSDB_intern_code_view_shortname ."` 
								WHERE `" . $RSDB_intern_code_view_shortname . "_visible` = '1'
								AND `" . $RSDB_intern_code_view_shortname . "_id` = " . $RSDB_SET_item . "
								ORDER BY `" . $RSDB_intern_code_view_shortname . "_name` ASC") ;
				$result_current_group = mysql_fetch_array($query_current_group);
				echo " <font size='2'>&gt;</font> <a href='".$RSDB_intern_link_item.$RSDB_SET_item."'>".$result_current_group[$RSDB_intern_code_view_shortname .'_name'];
				
					switch ($RSDB_SET_view) {
						case "comp": // Compatibility
						default:
							echo " ["."ReactOS ".show_osversion($result_current_group['comp_osversion'])."]";
							break;
						case "pack": // Packages
							echo " [".$result_current_group['pack_rosversion']."]";
							break;
						case "devnet": // Developer Network
							echo " [rev. ".$result_current_group['devnet_version']."]";
							break;
						case "media": // Media
							echo " [".$result_current_group['media_version']."]";
							break;
					}
				
				echo "</a>";
			}
			
		}
    echo "</td></tr></table>";
	include('inc/tree/tree_menubar_sections.php');	

	if ($RSDB_SET_group == "" && $RSDB_SET_item == "" && $RSDB_viewpage != false) {
		echo "<h2>".$RSDB_TEMP_current_category."</h2>";
		echo "<p>".$RSDB_TEMP_current_category_desc."</p>";
	}

	if ($RSDB_viewpage != false) {
		if ($RSDB_SET_group != "" || $RSDB_SET_item != "") {
			echo "<br />";
		}
		else {
			echo "<p align='center'>";
			if ($RSDB_SET_cat2 == "flat") {
				echo "<b>Flat Style</b> | <a href='".$RSDB_intern_link_category_cat2."tree'>Tree Style</a>";
			}
			if ($RSDB_SET_cat2 == "tree") {
				echo "<a href='".$RSDB_intern_link_category_cat2."flat'>Flat Style</a> | <b>Tree Style</b>";
			}
			echo "</p>";
		}
	}
	}
?>


<?php
	if ($RSDB_SET_sec == "name" || $RSDB_SET_sec == "vendor") {
		if ($RSDB_SET_sec == "name") {
			if ($RSDB_SET_item != "") {
				$query_itemid = mysql_query("SELECT * 
												FROM `rsdb_item_" . $RSDB_intern_code_view_shortname ."` 
												WHERE `" . $RSDB_intern_code_view_shortname . "_visible` = '1'
												AND `" . $RSDB_intern_code_view_shortname . "_id` = " . $RSDB_SET_item . "
												ORDER BY `" . $RSDB_intern_code_view_shortname . "_name` ASC") ;				
				$result_itempid = @mysql_fetch_array($query_itemid);			
				if ($result_itempid[$RSDB_intern_code_view_shortname.'_groupid'] == "" || $result_itempid[$RSDB_intern_code_view_shortname . '_groupid'] == "0") {
					$RSDB_viewpage = false;
				}
				$RSDB_SET_group = $result_itempid[$RSDB_intern_code_view_shortname . '_groupid'];
			}
			if ($RSDB_SET_group != "") {
			$query_groupid = mysql_query("SELECT * 
											FROM `rsdb_groups` 
											WHERE `grpentr_visible` = '1'
											AND `grpentr_id` = " . $RSDB_SET_group . "
											ORDER BY `grpentr_id` ASC") ;				
			$result_groupid = mysql_fetch_array($query_groupid);			
			$RSDB_SET_letter = strtolower(substr($result_groupid['grpentr_name'], 0, 1)); 
		}
	?>
	<table width="100%" border="1" cellpadding="3" cellspacing="0" bordercolor="#5984C3">
	  <tr>
		<td>&nbsp;<font size="2">Browsing &gt;&gt;</font> <a href="<?php echo $RSDB_intern_link_name_letter.$RSDB_SET_letter ?>"><?php
	
		if ($RSDB_SET_letter != "") {
			echo ucfirst($RSDB_SET_letter);
		}
		else {
			echo "All";
		}
		
		echo "</a>";
		 
		if ($RSDB_SET_item != "" && $RSDB_viewpage != false) {
			$query_itemid = mysql_query("SELECT * 
											FROM `rsdb_item_" . $RSDB_intern_code_view_shortname ."` 
											WHERE `" . $RSDB_intern_code_view_shortname . "_visible` = '1'
											AND `" . $RSDB_intern_code_view_shortname . "_id` = " . $RSDB_SET_item . "
											ORDER BY `" . $RSDB_intern_code_view_shortname . "_name` ASC") ;				
			$result_itempid = mysql_fetch_array($query_itemid);			
			if ($result_itempid[$RSDB_intern_code_view_shortname.'_groupid'] == "" || $result_itempid[$RSDB_intern_code_view_shortname . '_groupid'] == "0") {
				//die("");
				$RSDB_viewpage = false;
			}
			$RSDB_SET_group = $result_itempid[$RSDB_intern_code_view_shortname . '_groupid'];
		}
		if ($RSDB_SET_group != "" && $RSDB_viewpage != false) {
			$query_current_group = mysql_query("SELECT * 
							FROM `rsdb_groups` 
							WHERE `grpentr_visible` = '1'
							AND `grpentr_id` = " . $RSDB_SET_group . "
							" . $RSDB_intern_code_db_rsdb_groups . "
							ORDER BY `grpentr_name` ASC") ;
			$result_current_group = mysql_fetch_array($query_current_group);
			if ($result_current_group['grpentr_category'] == "" || $result_current_group['grpentr_category'] == "0") {
				//die("");
				$RSDB_viewpage = false;
			}
			echo " <font size='2'>&gt;</font> <a href='".$RSDB_intern_link_group.$RSDB_SET_group."'>".$result_current_group['grpentr_name']."</a>";
		}
		if ($RSDB_SET_item != "" && $RSDB_viewpage != false) {
			$query_current_group = mysql_query("SELECT * 
							FROM `rsdb_item_" . $RSDB_intern_code_view_shortname ."` 
							WHERE `" . $RSDB_intern_code_view_shortname . "_visible` = '1'
							AND `" . $RSDB_intern_code_view_shortname . "_id` = " . $RSDB_SET_item . "
							ORDER BY `" . $RSDB_intern_code_view_shortname . "_name` ASC") ;
			$result_current_group = mysql_fetch_array($query_current_group);
			echo " <font size='2'>&gt;</font> <a href='".$RSDB_intern_link_item.$RSDB_SET_item."'>".$result_current_group[$RSDB_intern_code_view_shortname .'_name'];
			
				switch ($RSDB_SET_view) {
					case "comp": // Compatibility
					default:
						echo " ["."ReactOS ".show_osversion($result_current_group['comp_osversion'])."]";
						break;
					case "pack": // Packages
						echo " [".$result_current_group['pack_rosversion']."]";
						break;
					case "devnet": // Developer Network
						echo " [rev. ".$result_current_group['devnet_version']."]";
						break;
					case "media": // Media
						echo " [".$result_current_group['media_version']."]";
						break;
				}
			
			echo "</a>";
		}
	
		echo "</td></tr></table>";
	
	}
	elseif ($RSDB_SET_sec == "vendor") {

			if ($RSDB_SET_vendor != "") {
				$query_itemid = mysql_query("SELECT * 
												FROM `rsdb_item_vendor` 
												WHERE `vendor_id` = " . $RSDB_SET_vendor . "
												ORDER BY `vendor_name` ASC") ;				
				$result_itempid = @mysql_fetch_array($query_itemid);			
				if ($result_itempid['vendor_id'] == "" || $result_itempid['vendor_id'] == "0") {
					$RSDB_viewpage = false;
				}
				$RSDB_SET_letter = strtolower(substr($result_itempid['vendor_name'], 0, 1)); 
			}
	?>
	<table width="100%" border="1" cellpadding="3" cellspacing="0" bordercolor="#5984C3">
	  <tr>
		<td>&nbsp;<font size="2">Browsing &gt;&gt; <a href="<?php echo $RSDB_intern_link_name_letter.$RSDB_SET_letter; ?>">
		
		<?php
	
		if ($RSDB_SET_letter != "") {
			echo ucfirst($RSDB_SET_letter);
		}
		else {
			echo "All";
		}
		
		echo "</a></font>";
		 
		if ($RSDB_SET_vendor != "" && $RSDB_viewpage != false) {
			$query_itemid = mysql_query("SELECT * 
												FROM `rsdb_item_vendor` 
												WHERE `vendor_id` = " . $RSDB_SET_vendor . "
												ORDER BY `vendor_name` ASC") ;				
			$result_itempid = mysql_fetch_array($query_itemid);			
			if ($result_itempid['vendor_id'] == "" || $result_itempid['vendor_id'] == "0") {
				//die("");
				$RSDB_viewpage = false;
			}
			//$RSDB_SET_group = $result_itempid['vendor_id'];
			echo " <font size='2'>&gt;</font> <a href='".$RSDB_intern_link_vendor.$RSDB_SET_vendor."'>".$result_itempid['vendor_name']."</a>";
		}
		if ($RSDB_SET_group != "" && $RSDB_viewpage != false) {
			$query_current_group = mysql_query("SELECT * 
							FROM `rsdb_groups` 
							WHERE `grpentr_visible` = '1'
							AND `grpentr_id` = " . $RSDB_SET_group . "
							" . $RSDB_intern_code_db_rsdb_groups . "
							ORDER BY `grpentr_name` ASC") ;
			$result_current_group = mysql_fetch_array($query_current_group);
			if ($result_current_group['grpentr_category'] == "" || $result_current_group['grpentr_category'] == "0") {
				//die("");
				$RSDB_viewpage = false;
			}
			echo " <font size='2'>&gt;</font> <a href='".$RSDB_intern_link_group.$RSDB_SET_group."'>".$result_current_group['grpentr_name']."</a>";
		}
		if ($RSDB_SET_item != "" && $RSDB_viewpage != false) {
			$query_current_group = mysql_query("SELECT * 
												FROM `rsdb_item_vendor` 
												WHERE `vendor_id` = " . $RSDB_SET_vendor . "
												ORDER BY `vendor_name` ASC") ;				
			$result_current_group = mysql_fetch_array($query_current_group);
			echo " <font size='2'>&gt;</font> <a href='".$RSDB_intern_link_vendor.$RSDB_SET_item."'>".$result_current_group['vendor_name'];
			echo "</a>";
		}
	
		echo "</td></tr></table>";

	}




	include('inc/tree/tree_menubar_sections.php');	

	if ($RSDB_SET_sec == "name") {
		$RSDB_TEMP_link_letter = $RSDB_intern_link_name_letter;
	}
	elseif ($RSDB_SET_sec == "vendor") {
		$RSDB_TEMP_link_letter = $RSDB_intern_link_name_letter;
	}

	if ($RSDB_viewpage != false) {

		echo '<p align="center">';
		
		if ($RSDB_SET_letter == "all" || $RSDB_SET_letter == "") {
			echo '  <b>All</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'all" class="letterbarlink">All</a> ';
		}
	
		if ($RSDB_SET_letter == "a") {
			echo '  <b>A</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'a" class="letterbarlink">A</a> ';
		}
	
		if ($RSDB_SET_letter == "b") {
			echo '  <b>B</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'b" class="letterbarlink">B</a> ';
		}
	
		if ($RSDB_SET_letter == "c") {
			echo '  <b>C</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'c" class="letterbarlink">C</a> ';
		}
	
		if ($RSDB_SET_letter == "d") {
			echo '  <b>D</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'d" class="letterbarlink">D</a> ';
		}
	
		if ($RSDB_SET_letter == "e") {
			echo '  <b>E</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'e" class="letterbarlink">E</a> ';
		}
	
		if ($RSDB_SET_letter == "f") {
			echo '  <b>F</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'f" class="letterbarlink">F</a> ';
		}
	
		if ($RSDB_SET_letter == "g") {
			echo '  <b>G</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'g" class="letterbarlink">G</a> ';
		}
	
		if ($RSDB_SET_letter == "h") {
			echo '  <b>H</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'h" class="letterbarlink">H</a> ';
		}
	
		if ($RSDB_SET_letter == "i") {
			echo '  <b>I</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'i" class="letterbarlink">I</a> ';
		}
	
		if ($RSDB_SET_letter == "j") {
			echo '  <b>J</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'j" class="letterbarlink">J</a> ';
		}
	
		if ($RSDB_SET_letter == "k") {
			echo '  <b>K</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'k" class="letterbarlink">K</a> ';
		}
	
		if ($RSDB_SET_letter == "l") {
			echo '  <b>L</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'l" class="letterbarlink">L</a> ';
		}
	
		if ($RSDB_SET_letter == "m") {
			echo '  <b>M</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'m" class="letterbarlink">M</a> ';
		}
		
		if ($RSDB_SET_letter == "n") {
			echo '  <b>N</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'n" class="letterbarlink">N</a> ';
		}
		
		if ($RSDB_SET_letter == "o") {
			echo '  <b>O</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'o" class="letterbarlink">O</a> ';
		}
	
		if ($RSDB_SET_letter == "p") {
			echo '  <b>P</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'p" class="letterbarlink">P</a> ';
		}
	
		if ($RSDB_SET_letter == "q") {
			echo '  <b>Q</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'q" class="letterbarlink">Q</a> ';
		}
	
		if ($RSDB_SET_letter == "r") {
			echo '  <b>R</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'r" class="letterbarlink">R</a> ';
		}
	
		if ($RSDB_SET_letter == "s") {
			echo '  <b>S</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'s" class="letterbarlink">S</a> ';
		}
	
		if ($RSDB_SET_letter == "t") {
			echo '  <b>T</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'t" class="letterbarlink">T</a> ';
		}
	
		if ($RSDB_SET_letter == "u") {
			echo '  <b>U</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'u" class="letterbarlink">U</a> ';
		}
	
		if ($RSDB_SET_letter == "v") {
			echo '  <b>V</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'v" class="letterbarlink">V</a> ';
		}
	
		if ($RSDB_SET_letter == "w") {
			echo '  <b>W</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'w" class="letterbarlink">W</a> ';
		}
	
		if ($RSDB_SET_letter == "x") {
			echo '  <b>X</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'x" class="letterbarlink">X</a> ';
		}
	
		if ($RSDB_SET_letter == "y") {
			echo '  <b>Y</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'y" class="letterbarlink">Y</a> ';
		}
	
		if ($RSDB_SET_letter == "z") {
			echo '  <b>Z</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_intern_link_name_letter .'z" class="letterbarlink">Z</a> ';
		}
	
		echo "</p><br />";

		}
	}
	
	if ($RSDB_viewpage == false) {
		echo "<p>No related database entry found!</p>";
	}
	
	
	switch ($RSDB_SET_view) {
		case "comp": // Compatibility
		default:
			//msg_bar("Compatibility");
			break;
		case "pack": // Packages
			msg_bar("The Package Section is under heavy construction!");
			echo "<br />";
			break;
		case "devnet": // Developer Network
			msg_bar("The Dev Network Section is under heavy construction!");
			echo "<br />";
			break;
		case "media": // Media
			msg_bar("The Media Section is under heavy construction!");
			echo "<br />";
			break;
	}
?>