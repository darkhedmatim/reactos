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



$query_count_cat=mysql_query("SELECT COUNT('cat_id')
						FROM `rsdb_categories`
						WHERE `cat_visible` = '1'
						AND `cat_path` = " . htmlentities($RSDB_SET_cat) . "
						" . $RSDB_intern_code_db_rsdb_categories . " ;");	
$result_count_cat = mysql_fetch_row($query_count_cat);

// Update the ViewCounter:
if ($RSDB_SET_cat != "" || $RSDB_SET_cat != "0") {
	$query_update_viewcounter = "UPDATE `rsdb_categories` SET `cat_viewcounter` = (cat_viewcounter + 1) WHERE `cat_id` = '" . $RSDB_SET_cat . "' LIMIT 1 ;";
	@mysql_query($query_update_viewcounter);
}

if ($result_count_cat[0]) {

	if ($RSDB_intern_code_view_shortname == "devnet") {
		$RSDB_TEMP_sortby = "cat_order";
	}
	else {
		$RSDB_TEMP_sortby = "cat_name";
	}

?>
	 <table width="100%" border="0" cellpadding="1" cellspacing="1">
	  <tr bgcolor="#5984C3"> 
		<td width="25%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Category</strong></font></div></td>
		<td width="65%" bgcolor="#5984C3"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Description</strong></font></div></td>
		<td width="10%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>No.</strong></font></div></td>
	  </tr>
	  <?php
	
		$query_page = mysql_query("SELECT * 
									FROM `rsdb_categories` 
									WHERE `cat_visible` = '1'
									AND `cat_path` = " . htmlentities($RSDB_SET_cat) . "
									" . $RSDB_intern_code_db_rsdb_categories . "
									ORDER BY `".htmlentities($RSDB_TEMP_sortby)."` ASC") ;
	
		$farbe1="#E2E2E2";
		$farbe2="#EEEEEE";
		$zaehler="0";
		//$farbe="#CCCCC";
		
		include('inc/tree/tree_category_flat_count_grouplist.php');

		
		while($result_page = mysql_fetch_array($query_page)) { // Pages
	?>
	  <tr> 
		<td width="25%" valign="top" bgcolor="<?php
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
								 ?>" > <div align="left"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<b><a href="<?php echo $RSDB_intern_link_category_cat.$result_page['cat_id']; ?>"><?php echo $result_page['cat_name']; ?></a></b></font></div></td>
		<td width="65%" valign="top" bgcolor="<?php echo $farbe; ?>"> <div align="left"><font face="Arial, Helvetica, sans-serif"><font size="2"> 
        </font><font face="Arial, Helvetica, sans-serif"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_page['cat_description']; ?></font></font><font size="2"> 
        </font> </font></div></td>
		
    <td width="10%" valign="top" bgcolor="<?php echo $farbe; ?>"> <font face="Arial, Helvetica, sans-serif"><font size="2">
      <?php

		echo count_tree_grouplist($result_page['cat_id']);
	
	?>
      </font></font></td>
	  </tr>
	  <?php	
		}	// end while
	?>
	</table>
	<p>&nbsp;</p>
<?php
}
?>

