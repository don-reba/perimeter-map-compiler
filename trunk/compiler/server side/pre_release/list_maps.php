<?

// definitions
define('MAP_LIST_PATH', 'map_list.dat');
define('FAILURE_MSG',   'failure');

// check user agent
if ($HTTP_USER_AGENT != 'Perimeter Map Compiler')
	exit(FAILURE_MSG);

// read the map list
$map_list = file_get_contents(MAP_LIST_PATH);
if (!$map_list)
	exit(FAILURE_MSG);
$map_list = unserialize($map_list);
if (!$map_list)
	$map_list = array();

// output the map list
foreach ($map_list as $map)
	echo $map['name'] . "\n";

?>