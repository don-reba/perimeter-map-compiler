<?

// definitions
define('MAP_LIST_PATH',     'map_list.dat');
define('INVALID_INDEX_MSG', 'invalid map index');
define('OUT_OF_BOUNDS_MSG', 'map index out of bounds');
define('SUCCESS_MSG',       'success');

// read the map list
$map_list = array_values(unserialize(file_get_contents(MAP_LIST_PATH)));

// validate map_index
if (!isset($map_index) || !is_numeric($map_index))
	exit(INVALID_INDEX_MSG);
--$map_index;
if ($map_index < 0 || $map_index >= count($map_list))
	exit(OUT_OF_BOUNDS_MSG);

// remove map at index
unset($map_list[$map_index]);

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