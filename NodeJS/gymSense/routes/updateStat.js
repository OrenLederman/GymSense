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
  if (('sensorId' in  req.query) && 'activityCount' in req.query) {
    sensorId = req.query['sensorId'];
    activityCount = req.query['activityCount'];
    var query = connection.query('insert into sensor_data (ts,sensor_id,activity_cnt) values (now(),'+sensorId+','+activityCount+')' , function(err, result) {
      if (err) throw err;
      //console.log('The solution is: ', rows[0].solution);
      //console.log('inserted');
    });
    // looks like we don't need this - connection.end();
    res.send('got it');
  } else {
     res.status(400);
     res.send('missing something');
  }
});

module.exports = router;
