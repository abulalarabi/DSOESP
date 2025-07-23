void initWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    int usagePercent = (100 * usedBytes) / totalBytes;

    String html = R"rawliteral(
      <!DOCTYPE html>
      <html lang="en">
      <head>
        <meta charset="UTF-8">
        <title>DSO ESP</title>
        <script src='https://cdn.plot.ly/plotly-latest.min.js'></script>
        <style>
          body {
            font-family: 'Segoe UI', sans-serif;
            background: linear-gradient(to right, #f0f0f0, #e0e0e0);
            color: #333;
            margin: 0;
            padding: 20px;
          }
          .container {
            max-width: 700px;
            margin: auto;
            background: white;
            padding: 25px 30px;
            border-radius: 10px;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
          }
          header {
            text-align: center;
            margin-bottom: 20px;
          }
          h2 {
            margin: 0;
            font-size: 2em;
            color: #222;
          }
          .subtitle {
            margin: 0;
            font-size: 1em;
            color: #888;
          }
          h3 {
            margin-top: 0;
            color: #444;
            border-bottom: 2px solid #ddd;
            padding-bottom: 5px;
          }
          .storage-box {
            margin: 15px 0 25px 0;
          }
          .storage-label {
            font-size: 0.95em;
            color: #444;
            margin-bottom: 6px;
          }
          .progress {
            width: 100%;
            background-color: #ddd;
            border-radius: 10px;
            overflow: hidden;
            height: 20px;
            box-shadow: inset 0 1px 3px rgba(0,0,0,0.1);
          }
          .bar {
            height: 100%;
            background-color: #007bff;
            width: 0%;
            transition: width 0.6s ease;
          }
          .file-list {
            list-style: none;
            padding: 0;
            margin: 0;
          }
          .file-list li {
            background: #f8f8f8;
            margin: 8px 0;
            padding: 10px 14px;
            border-radius: 6px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            box-shadow: 0 2px 5px rgba(0,0,0,0.05);
            flex-wrap: wrap;
          }
          .file-info {
            flex-grow: 1;
            font-weight: 500;
          }
          .file-info small {
            font-weight: normal;
            color: #777;
          }
          .file-actions a {
            text-decoration: none;
            padding: 6px 10px;
            margin-left: 8px;
            font-size: 0.9em;
            border-radius: 4px;
            color: white;
          }
          .download { background-color: #28a745; }
          .delete { background-color: #dc3545; }
          .plot { background-color: #17a2b8; }
          .download:hover { background-color: #218838; }
          .delete:hover { background-color: #c82333; }
          .plot:hover { background-color: #138496; }
        </style>
        <script>
          function confirmDelete(file) {
            if (confirm("Are you sure you want to delete " + file + "?")) {
              window.location.href = "/confirmdelete?name=" + file;
            }
          }
          function confirmDeleteAll() {
            if (confirm("⚠️ This will permanently delete all files. Proceed?")) {
              window.location.href = "/deleteall";
            }
          }
        </script>
      </head>
      <body>
        <div class="container">
          <header>
            <h2>📟 DSO ESP</h2>
            <p class="subtitle">by Arabi</p>
          </header>
          <section>
            <div style="display: flex; justify-content: space-between; align-items: center;">
              <h3 style="margin: 0;">📁 Saved Files</h3>
              <span class='file-actions'>
                <a class='delete' href='javascript:void(0);' onclick='confirmDeleteAll()'>Delete All</a>
              </span>
            </div>
    )rawliteral";

    html += "<div class='storage-box'>"
            "<div class='storage-label'>SPIFFS Usage: "
            + String(usedBytes / 1024.0, 2) + " KB / " + String(totalBytes / 1024.0, 2) + " KB (" + String(usagePercent) + "%)</div>"
                                                                                                                           "<div class='progress'><div class='bar' style='width:"
            + String(usagePercent) + "%'></div></div></div>";

    html += "<ul class='file-list'>";

    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
      String displayName = String(file.name());
      if (displayName.startsWith("/")) displayName = displayName.substring(1);

      size_t size = file.size();
      String sizeStr = size >= 1024 ? String(size / 1024.0, 2) + " KB" : String(size) + " B";

      html += "<li><span class='file-info'>" + displayName + " <small>(" + sizeStr + ")</small></span><span class='file-actions'>"
                                                                                     "<a class='download' href='/download?name="
              + displayName + "'>Download</a>"
                              "<a class='delete' href='javascript:void(0);' onclick='confirmDelete(\""
              + displayName + "\")'>Delete</a>"
                              "<a class='plot' href='/plot?name="
              + displayName + "'>Plot</a></span></li>";

      file = root.openNextFile();
    }

    html += R"rawliteral(
            </ul>
          </section>
        </div>
      </body>
      </html>
    )rawliteral";

    request->send(200, "text/html", html);
  });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("name")) {
      String fileName = "/" + request->getParam("name")->value();
      if (LittleFS.exists(fileName)) {
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, fileName, "text/csv");
        response->addHeader("Content-Disposition", "attachment; filename=" + fileName.substring(1));
        request->send(response);
      } else {
        request->send(404, "text/plain", "File not found");
      }
    } else {
      request->send(400, "text/plain", "Missing filename");
    }
  });

  server.on("/confirmdelete", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("name")) {
      String fileName = "/" + request->getParam("name")->value();
      if (LittleFS.exists(fileName)) {
        LittleFS.remove(fileName);
        findLastFileIndex();
      }
      request->redirect("/");
    } else {
      request->send(400, "text/plain", "Missing filename");
    }
  });

  server.on("/deleteall", HTTP_GET, [](AsyncWebServerRequest *request) {
    LittleFS.format();
    nextFileIndex = 0;
    request->redirect("/");
  });


  server.on("/plot", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("name")) {
      request->send(400, "text/plain", "Missing filename");
      return;
    }

    String fileName = "/" + request->getParam("name")->value();
    if (!LittleFS.exists(fileName)) {
      request->send(404, "text/plain", "File not found");
      return;
    }

    File file = LittleFS.open(fileName);
    if (!file) {
      request->send(500, "text/plain", "Failed to open file");
      return;
    }

    String html = "<!DOCTYPE html><html><head><title>Plot</title>"
                  "<script src='https://cdn.plot.ly/plotly-latest.min.js'></script>"
                  "<style>body{font-family:sans-serif;padding:20px;background:#f0f0f0;}"
                  ".plot-card{background:#fff;padding:20px;border-radius:10px;max-width:900px;margin:auto;box-shadow:0 4px 10px rgba(0,0,0,0.1);}"
                  "#plot{height:500px;}</style></head><body>"
                  "<div class='plot-card'><a href='/'>&larr; Back</a><h2>Plot of "
                  + fileName + "</h2><div id='plot'></div></div><script>";

    html += "let csvData = `";

    // Read file line-by-line and embed waveform data only
    while (file.available()) {
      String line = file.readStringUntil('\n');
      line.trim();

      if (line.length() > 0 && line.charAt(0) >= '0' && line.charAt(0) <= '9') {
        // likely a waveform row like 00001,0000000020,-0.04
        html += line + "\\n";
      }
    }

    file.close();

    html += "`.trim().split('\\n');"
            "let t=[],v=[];"
            "csvData.forEach(l=>{"
            "let p=l.split(',');"
            "if(p.length===3){"
            "  t.push(parseFloat(p[1])/1000);"
            "  v.push(parseFloat(p[2]));"
            "}});"
            "if(t.length===0||v.length===0){"
            "document.getElementById('plot').innerHTML='<p style=\"color:red;\">⚠️ No valid data found.</p>';"
            "}else{"
            "Plotly.newPlot('plot',[{x:t,y:v,type:'scatter',mode:'lines',line:{color:'#007bff'}}],"
            "{margin:{t:30},xaxis:{title:'Time (ms)'},yaxis:{title:'Voltage (V)'}});}"
            "</script></body></html>";

    request->send(200, "text/html", html);
  });



  server.begin();
}
