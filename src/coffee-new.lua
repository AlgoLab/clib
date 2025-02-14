function check_dir(path)
	  print("Lua argument" .. path)
  local file = io.open(path, "r") -- Try to open the path for reading

  if file then
    file:close() -- Close the file if it was opened
	print("Found path '" .. path) -- Print the error if needed
    return 0 -- Path exists
  else
	print("Not Found path '" .. path) -- Print the error if needed
    return 1 -- Path does not exist (io.open returns nil if it fails)
  end
end
