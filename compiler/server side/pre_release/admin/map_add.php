<?

// definitions
define('MAP_LIST_PATH',     '../map_list.dat');
define('INVALID_NAME_MSG', 'invalid map name');
define('SUCCESS_MSG',       'success');

// read the map list
$map_list = array_values(unserialize(file_get_contents(MAP_LIST_PATH)));

// validate map_name
if (!isset($map_name) || !is_string($map_name) || empty($map_name))
	exit(INVALID_INDEX_MSG);

// add map with this name
array_push($map_list, array('name' => $map_name, 'checksum' => "-"));

// write the map list
$file = fopen(MAP_LIST_PATH, 'w');
if ($file)
{
	fwrite($file, serialize($map_list));
	fclose($file);
}

// reinforce the user
echo SUCCESS_MSG;

?>