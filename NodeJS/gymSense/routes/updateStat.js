var express = require('express');
var router = express.Router();

var mysql      = require('mysql');
var connection = mysql.createConnection({
  host     : 'localhost',
  user     : 'gymsense',
  password : 'gymsense',
  database : 'gymsense'
});

/* GET users listing. */
router.get('/', function(req, res, next) {
  //console.log('sensor id: ' + req.query['sensorId']);
  if (('sensorId' in  req.query)) {
    connection.connect();

    sensorId = req.query['sensorId'];
    var query = connection.query('insert into sensor_data (ts,sensor_id) values (now(),'+sensorId+')' , function(err, result) {
      if (err) throw err;
      //console.log('The solution is: ', rows[0].solution);
      //console.log('inserted');
    });
    connection.end();
    res.send('got it');
  } else {
     res.send('missing something');
  }
});

module.exports = router;
