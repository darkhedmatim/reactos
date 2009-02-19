    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2009  Danny G�tte <dangerground@web.de>

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

// search filter standard text
clearSearchFilter();

// open standard left menu
loadUserSearch();

// load user filter
document.getElementById('userfilter2c').innerHTML = '<div align="right"><img src="images/ajax_loading.gif" alt="loading ..." style="width:13px; height:13px;" /></div>';
makeRequest('?page=backend&type=text&subtype=usf&d_val=load', 'usf', 'userfilter2c', 'html', 'GET', '');

if (readCookie('userfilter') == 0) TabOpenCloseEx('userfilter');

roscms_page_load_finished = true;

// window unload blocker
if (exitmsg !== '') {
  window.onbeforeunload = exitmsg;
}
else {
  window.onbeforeunload = false;
}