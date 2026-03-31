const express = require('express');
const cors = require('cors');

const runRoute = require('./routes/run');

const app = express();

app.use(cors());
app.use(express.json({ limit: '100kb' }));

app.get('/api/health', (req, res) => {
  	res.json({ status: 'ok' });
});

app.use('/api/run', runRoute);

module.exports = app;