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
          .download {
            background-color: #28a745;
          }
          .delete {
            background-color: #dc3545;
          }
          .plot {
            background-color: #17a2b8;
          }
          .download:hover {
            background-color: #218838;
          }
          .delete:hover {
            background-color: #c82333;
          }
          .plot:hover {
            background-color: #138496;
          }
          .toast {
            position: fixed;
            bottom: 20px;
            right: 20px;
            background: #28a745;
            color: #fff;
            padding: 10px 16px;
            border-radius: 4px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.2);
            display: none;
            z-index: 1000;
          }
        </style>
        <script>
          function confirmDelete(file) {
            if (confirm("Are you sure you want to delete " + file + "?")) {
              window.location.href = "/confirmdelete?name=" + file;
            }
          }
          function showToast(msg) {
            const toast = document.getElementById("toast");
            toast.textContent = msg;
            toast.style.display = "block";
            setTimeout(() => { toast.style.display = "none"; }, 3000);
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
            <h3>📁 Saved Files</h3>
  )rawliteral";

    html += "<div class='storage-box'>"
            "<div class='storage-label'>SPIFFS Usage: " +
            String(usedBytes / 1024.0, 2) + " KB / " +
            String(totalBytes / 1024.0, 2) + " KB (" +
            String(usagePercent) + "%)</div>"
            "<div class='progress'><div class='bar' style='width:" +
            String(usagePercent) + "%'></div></div></div>";

    html += "<ul class='file-list'>";

    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
      String fullPath = String(file.name());
      String displayName = fullPath.startsWith("/") ? fullPath.substring(1) : fullPath;
      size_t size = file.size();

      String sizeStr;
      if (size >= 1024) {
        sizeStr = String(size / 1024.0, 2) + " KB";
      } else {
        sizeStr = String(size) + " B";
      }

      html += "<li><span class='file-info'>" + displayName + " <small>(" + sizeStr + ")</small></span><span class='file-actions'>" +
              "<a class='download' href='/download?name=" + displayName + "'>Download</a>" +
              "<a class='delete' href='javascript:void(0);' onclick='confirmDelete(\"" + displayName + "\")'>Delete</a>" +
              "<a class='plot' href='/plot?name=" + displayName + "'>Plot</a></span></li>";

      file = root.openNextFile();
    }

    html += R"rawliteral(
            </ul>
          </section>
        </div>
        <div id="toast" class="toast"></div>
      </body>
      </html>
    )rawliteral";

    request->send(200, "text/html", html);
  });

  server.on("/plot", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("name")) {
      String fileName = request->getParam("name")->value();
      if (!fileName.startsWith("/")) fileName = "/" + fileName;
      if (LittleFS.exists(fileName)) {
        File file = LittleFS.open(fileName);
        String csv = file.readString();
        file.close();

        String html = "<html><head><title>Plot " + fileName + "</title>"
                      "<script src='https://cdn.plot.ly/plotly-latest.min.js'></script>"
                      "<style>body { font-family: 'Segoe UI', sans-serif; background: #f0f2f5; color: #333; padding: 20px; margin: 0; }"
                      "h2 { margin: 10px 0 20px; text-align: center; } .plot-card { background: #fff; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); padding: 20px; max-width: 900px; margin: auto; }"
                      "#plot { height: 500px; } .back-btn { display: block; margin-bottom: 10px; color: #007bff; text-decoration: none; font-size: 0.95em; } .back-btn:hover { text-decoration: underline; }"
                      "</style></head><body>"
                      "<div class='plot-card'><a href='/' class='back-btn'>&larr; Back</a><h2>Plot: " + fileName + "</h2><div id='plot'></div></div>"
                      "<script>let csv = `" + csv + "`\n.split(`\n`).filter(l=>l.trim().length>0).slice(1);"
                      "let t = [], v = [];csv.forEach(l=>{let p=l.split(',');t.push(p[0]);v.push(parseFloat(p[1]));});"
                      "Plotly.newPlot('plot', [{x: t, y: v, type: 'scatter', mode: 'lines+markers', marker: {color: '#007bff'}, name: 'Voltage'}],"
                      "{margin: { t: 30 }, xaxis: {title: {text: 'Time', font: {size: 18}}}, yaxis: {title: {text: 'Voltage', font: {size: 18}}}, hovermode: 'closest',"
                      "plot_bgcolor: '#f9f9f9', paper_bgcolor: '#ffffff', font: {family: 'Segoe UI', size: 14, color: '#333'}});"
                      "</script></body></html>";
        request->send(200, "text/html", html);
      } else {
        request->send(404, "text/plain", "File not found");
      }
    } else {
      request->send(400, "text/plain", "File name not provided");
    }
  });

  // Download and Delete routes remain unchanged...
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("name")) {
      String fileName = request->getParam("name")->value();
      if (!fileName.startsWith("/")) fileName = "/" + fileName;
      if (LittleFS.exists(fileName)) {
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, fileName, "text/csv");
        response->addHeader("Content-Disposition", "attachment; filename=" + fileName.substring(1));
        request->send(response);
      } else {
        request->send(404, "text/plain", "File not found");
      }
    } else {
      request->send(400, "text/plain", "File name not provided");
    }
  });

  server.on("/confirmdelete", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("name")) {
      String fileName = request->getParam("name")->value();
      if (!fileName.startsWith("/")) fileName = "/" + fileName;

      if (LittleFS.exists(fileName)) {
        LittleFS.remove(fileName);
        findLastFileIndex();
      }

      String html = "<html><body><p>Deleting file: " + fileName + "</p>"
                    "<script>window.onload=function(){"
                    "setTimeout(()=>{window.location.href='/'}, 1000);"
                    "};</script></body></html>";
      request->send(200, "text/html", html);
    } else {
      request->send(400, "text/plain", "File name not provided");
    }
  });

  server.begin();
}