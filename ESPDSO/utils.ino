void findLastFileIndex() {
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    String name = String(file.name());
    if (name.startsWith("/data_") && name.endsWith(".csv")) {
      int underscore = name.indexOf('_');
      int dot = name.lastIndexOf('.');
      if (underscore >= 0 && dot > underscore) {
        String numStr = name.substring(underscore + 1, dot);
        int num = numStr.toInt();
        if (num >= nextFileIndex) {
          nextFileIndex = num + 1;
        }
      }
    }
    file = root.openNextFile();
  }
}