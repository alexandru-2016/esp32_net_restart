const express = require("express");
var bodyParser = require("body-parser");

const app = express();

app.use(bodyParser.urlencoded({ extended: false }));
app.use(bodyParser.json());

app.get("/update", (req, res) => {
    if (!req.header("x-esp32-version")) {
      res.sendStatus(400);
      return;
    }

    console.log("Esp32 request: " + req.header("x-esp32-sta-mac"));

    // check if update is required based on version received
    if (req.header("x-esp32-version") == "0.03") {
      console.log("Node is up to date");
      res.sendStatus(304);
      return;
    }
  
    // log what node we update
    console.log("Updating node");
  
    const file = `${__dirname}/net_restart.ino.d32.bin`;
    res.download(file); // Set disposition and send it.
  });

const port = 8100;

const server = app.listen(port, () =>
    console.log(`Update server listening on port ${port}!`)
);