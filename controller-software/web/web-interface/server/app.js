// server/app.js

const express = require('express');
const path = require('path');
const bodyParser = require('body-parser');
const session = require('express-session');
const passport = require('passport');
const LocalStrategy = require('passport-local').Strategy;
const { initWebSocketServer } = require('./wsServer');
const http = require('http');

const app = express();

// ===== Middleware Setup =====

// Parse URL-encoded bodies (as sent by HTML forms)
app.use(bodyParser.urlencoded({ extended: false }));

// Setup session management (for demo, a hard-coded secret is used)
app.use(session({
  secret: 'your_secret_key', // In production, use a secure, unpredictable secret
  resave: false,
  saveUninitialized: false,
}));

// Initialize Passport and use sessions
app.use(passport.initialize());
app.use(passport.session());

// ===== Fake User Database =====

// For this example, we’ll use an in-memory user “database”.
// In a real application, replace this with your user data store.
const users = [
    { id: 1, username: 'admin', password: 'admin' },
    { id: 2, username: 'user', password: 'user' },
  // You can add more users here
];

// ===== Passport Local Strategy =====

passport.use(new LocalStrategy((username, password, done) => {
  const user = users.find(u => u.username === username);
  if (!user) {
    return done(null, false, { message: 'Incorrect username.' });
  }
  if (user.password !== password) {
    return done(null, false, { message: 'Incorrect password.' });
  }
  return done(null, user);
}));

passport.serializeUser((user, done) => {
  done(null, user.id);
});

passport.deserializeUser((id, done) => {
  const user = users.find(u => u.id === id);
  done(null, user);
});

// ===== Routes =====

// Home page: if logged in, redirect to SPA; otherwise, show the login page.
app.get('/', (req, res) => {
  if (req.isAuthenticated() || 1) { // For demo purposes, we’ll always redirect to the SPA
    return res.redirect('/spa');
  }
  res.sendFile(path.join(__dirname, '..', 'client', 'login/login.html'));
});

// Login handler: authenticate using Passport.
app.post('/login', passport.authenticate('local', {
  successRedirect: '/spa',
  failureRedirect: '/',  // In a real app, you might want to show a message on failure.
}));

// Middleware to ensure a route is only accessible when authenticated.
function ensureAuthenticated(req, res, next) {
  if (req.isAuthenticated()) {
    return next();
  }
  res.redirect('/');
}

// The test SPA: only accessible if the user is logged in.
app.get('/spa', (req, res) => { // skip ensureAuthenticated for demo purposes
  res.sendFile(path.join(__dirname, '..', 'client', 'spa_1/spa.html'));
});

// Logout route: end the session and redirect to login.
app.get('/logout', (req, res, next) => {
  req.logout(err => {
    if (err) { return next(err); }
    res.redirect('/');
  });
});

// Serve any static assets from the /client folder.
app.use(express.static(path.join(__dirname, '..', 'client')));


// Create HTTP server from Express
const server = http.createServer(app);

// Initialize the WebSocket server
initWebSocketServer(server);

// Finally, start listening
const PORT = process.env.PORT || 3000;
server.listen(PORT, '0.0.0.0', () => {
  console.log(`Server listening on port ${PORT}`);
});