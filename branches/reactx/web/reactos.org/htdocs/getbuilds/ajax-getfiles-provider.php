<?php
/*
  PROJECT:    ReactOS Website
  LICENSE:    GPL v2 or any later version
  FILE:       web/reactos.org/htdocs/getbuilds/ajax-getfiles-provider.php
  PURPOSE:    Easily download prebuilt ReactOS Revisions
  COPYRIGHT:  Copyright 2007 Colin Finck <mail@colinfinck.de>
*/
   
	// This "ajax-getfiles.php" script has to be uploaded to the server, which contains the ISO files.
	// Therefore it has an own configuration and doesn't use "config.inc.php".

	// Configuration
	$ISO_DIR = ".";
	$MAX_FILES_PER_PAGE = 100;			// The same value has to be set in "config.inc.php"
	$REV_RANGE_LIMIT = 3000;

	// Functions
	function fsize_str( $size )
	{
		if( $size > 1000000 )
		{
			$size = $size / 1048576;
			$unit = " MB";
		}
		else if( $size > 1000 )
		{
			$size = $size / 1024;
			$unit = " KB";
		}
		else
			$unit = " Bytes";
		
		return number_format( $size, 2, ".", ",") . $unit;
	}
	
	
	// Entry point
	if( !isset( $_GET["get"] ) || !isset( $_GET["startrev"] ) )
		die("Necessary information not specified!");
	
	if( $_GET["endrev"] - $_GET["startrev"] > $REV_RANGE_LIMIT )
		die("Maximum revision range limit of $REV_RANGE_LIMIT exceeded!");
	
	switch( $_GET["get"] )
	{
		case "all":
			$get_infos = true;
			$get_filelist = true;
			break;
		
		case "infos":
			$get_infos = true;
			break;
		
		case "filelist":
			$get_filelist = true;
			break;
		
		default:
			die("Wrong input for parameter 'get'!");
	}
	
	$file_patterns = array();
	if( $_GET["bootcd-dbg"] == 1 )
		$file_patterns[] = "#bootcd-[0-9]+-dbg#";
	if( $_GET["livecd-dbg"] == 1 )
		$file_patterns[] = "#livecd-[0-9]+-dbg#";
	if( $_GET["bootcd-rel"] == 1 )
		$file_patterns[] = "#bootcd-[0-9]+-rel#";
	if( $_GET["livecd-rel"] == 1 )
		$file_patterns[] = "#livecd-[0-9]+-rel#";
	
	header("Content-type: text/xml");
	echo "<fileinformation>";
	
	$exitloop = false;
	$filenum = 0;
	$firstrev = 0;
	$lastrev = 0;
	$morefiles = 0;
	$dir = opendir( $ISO_DIR ) or die("opendir failed!");

	while( $fname = readdir($dir) )
		if( preg_match( "#-([0-9]+)-#", $fname, $matches ) )
			$fnames[ $matches[1] ][] = $fname;
	
	closedir($dir);
	
	for( $i = $_GET["startrev"]; $i <= $_GET["endrev"]; $i++ )
	{
		if( isset( $fnames[$i] ) )
		{
			sort( $fnames[$i] );
			
			foreach( $fnames[$i] as $fname )
			{
				// Is it an allowed CD Image type?
				foreach( $file_patterns as $p )
				{
					if( preg_match( $p, $fname ) )
					{
						// This is a file we are looking for
						if( $get_filelist )
						{
							echo "<file>";
							printf("<name>%s</name>", $fname );
							printf("<size>%s</size>", fsize_str( filesize( "$ISO_DIR/$fname" ) ) );
							printf("<date>%s</date>", date( "Y-m-d H:i", filemtime( "$ISO_DIR/$fname" ) ) );
							echo "</file>";
						}
					
						if( $i < $firstrev || $firstrev == 0 )
							$firstrev = $i;
				
						if( $i > $lastrev )
							$lastrev = $i;
						
						$filenum++;
						break;
					}
				}
				
				if( $filenum == $MAX_FILES_PER_PAGE )
				{
					$morefiles = 1;
					$exitloop = true;
					break;
				}
			}
		}
		
		if( $exitloop )
			break;
	}
	
	if( $get_infos )
	{
		printf( "<filenum>%s</filenum>", $filenum );
		printf( "<firstrev>%s</firstrev>", $firstrev );
		printf( "<lastrev>%s</lastrev>", $lastrev );
		printf( "<morefiles>%s</morefiles>", $morefiles );
	}
	
	echo "</fileinformation>";
?>
