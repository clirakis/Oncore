Next to do. It appears that there are some cases where the 0xd 0xa message 
truncates too soon. Steps:

1) insert process to show how many bytes dropped before the next BOM. 
2) Use the prefix to determine how many bytes to process - kind of a major
   paradigm change. 
