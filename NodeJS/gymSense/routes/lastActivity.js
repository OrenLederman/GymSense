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
  sql = "select \
	max(case when sensor_id = 1 then ts else null end) as ts1 \
	,max(case when sensor_id = 2 then ts else null end) as ts2 \
	from sensor_data where activity_cnt > 0"
  var lastE = "";
  var lastT = "empty";
  var query = connection.query(sql, function(err, rows, fields) {
     if (err) throw err;
     console.log('Last sensor 1: ', rows[0].ts1);
     console.log('Last sensor 2: ', rows[0].ts2);
     lastE =  rows[0].ts1;
     lastT =  rows[0].ts2;
     console.log(lastE);
     console.log(lastT);
     res.render('lastActivity', { title: 'The Warehouse Gym status' , lastElliptical: lastE, lastTreadmill: lastT});
  });
});

module.exports = router;
