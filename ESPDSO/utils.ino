void findLastFileIndex() {
  nextFileIndex = 0;

  // — assume FS is already mounted in setup()
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) {
    return;
  }

  File file = root.openNextFile();
  while (file) {
    // strip leading slash if present
    String name = file.name();
    if (name.startsWith("/")) {
      name = name.substring(1);
    }

    // match data_<digits>.csv
    if (name.startsWith("data_") && name.endsWith(".csv")) {
      int u = name.indexOf('_');
      int d = name.lastIndexOf('.');
      String numStr = name.substring(u + 1, d);
      int n = numStr.toInt();
      if (n >= nextFileIndex) {
        nextFileIndex = n + 1;
      }
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
}
