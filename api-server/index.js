const express = require('express');
const jwt = require('jsonwebtoken');
const bcrypt = require('bcryptjs');
const cors = require('cors');

const app = express();
const PORT = process.env.PORT || 3000;
const JWT_SECRET = process.env.JWT_SECRET || 'your-secret-key-change-in-production';

// Middleware
app.use(cors({
  origin: 'http://localhost:5173',
  credentials: true
}));
app.use(express.json());

let users = [
  {
    id: 1,
    username: 'admin',
    email: 'admin@example.com',
    password: '$2b$10$QWSh0Ic1/15MDKtfkOF2BeYyxeN4oikb.CChLAnquTgeTS.SNs3c6', // 'admin123'
    createdAt: '2024-01-01T00:00:00Z'
  }
];

let serverStartTime = Date.now();
let activeConnections = 12;

// Authentication middleware
const authenticateToken = (req, res, next) => {
  const authHeader = req.headers['authorization'];
  const token = authHeader && authHeader.split(' ')[1];

  if (!token) {
    return res.status(401).json({ message: 'Access token required' });
  }

  jwt.verify(token, JWT_SECRET, (err, user) => {
    if (err) {
      return res.status(403).json({ message: 'Invalid or expired token' });
    }
    req.user = user;
    next();
  });
};

// Routes

// GET /api/health
app.get('/api/health', (req, res) => {
  res.json({ status: 'OK', timestamp: new Date().toISOString() });
});

// POST /api/auth/login
app.post('/api/auth/login', async (req, res) => {
  try {
    const { username, password } = req.body;

    if (!username || !password) {
      return res.status(400).json({ message: 'Username and password required' });
    }

    const user = users.find(u => u.username === username);
    if (!user) {
      return res.status(401).json({ message: 'Invalid credentials' });
    }

    const isValidPassword = await bcrypt.compare(password, user.password);
    if (!isValidPassword) {
      return res.status(401).json({ message: 'Invalid credentials' });
    }

    const token = jwt.sign(
      { id: user.id, username: user.username, email: user.email },
      JWT_SECRET,
      { expiresIn: '24h' }
    );

    res.json({
      token,
      user: {
        id: user.id,
        username: user.username,
        email: user.email
      }
    });
  } catch (error) {
    res.status(500).json({ message: 'Internal server error' });
  }
});

// GET /api/auth/me
app.get('/api/auth/me', authenticateToken, (req, res) => {
  const user = users.find(u => u.id === req.user.id);
  if (!user) {
    return res.status(404).json({ message: 'User not found' });
  }

  res.json({
    id: user.id,
    username: user.username,
    email: user.email
  });
});

// GET /api/users
app.get('/api/users', authenticateToken, (req, res) => {
  const userList = users.map(user => ({
    id: user.id,
    username: user.username,
    email: user.email,
    createdAt: user.createdAt
  }));

  res.json(userList);
});

// POST /api/users
app.post('/api/users', authenticateToken, async (req, res) => {
  try {
    const { username, email, password } = req.body;

    if (!username || !email || !password) {
      return res.status(400).json({ message: 'Username, email, and password required' });
    }

    // Check if user already exists
    if (users.find(u => u.username === username || u.email === email)) {
      return res.status(409).json({ message: 'User already exists' });
    }

    const hashedPassword = await bcrypt.hash(password, 10);
    const newUser = {
      id: users.length + 1,
      username,
      email,
      password: hashedPassword,
      createdAt: new Date().toISOString()
    };

    users.push(newUser);

    res.status(201).json({
      id: newUser.id,
      username: newUser.username,
      email: newUser.email,
      createdAt: newUser.createdAt
    });
  } catch (error) {
    res.status(500).json({ message: 'Internal server error' });
  }
});

// PUT /api/users/:id
app.put('/api/users/:id', authenticateToken, async (req, res) => {
  try {
    const userId = parseInt(req.params.id);
    const { username, email, password } = req.body;

    const userIndex = users.findIndex(u => u.id === userId);
    if (userIndex === -1) {
      return res.status(404).json({ message: 'User not found' });
    }

    // Update user
    if (username) users[userIndex].username = username;
    if (email) users[userIndex].email = email;
    if (password) {
      users[userIndex].password = await bcrypt.hash(password, 10);
    }

    res.json({
      id: users[userIndex].id,
      username: users[userIndex].username,
      email: users[userIndex].email,
      createdAt: users[userIndex].createdAt
    });
  } catch (error) {
    res.status(500).json({ message: 'Internal server error' });
  }
});

// DELETE /api/users/:id
app.delete('/api/users/:id', authenticateToken, (req, res) => {
  try {
    const userId = parseInt(req.params.id);

    // Don't allow deleting the admin user
    if (userId === 1) {
      return res.status(403).json({ message: 'Cannot delete admin user' });
    }

    const userIndex = users.findIndex(u => u.id === userId);
    if (userIndex === -1) {
      return res.status(404).json({ message: 'User not found' });
    }

    users.splice(userIndex, 1);
    res.json({ message: 'User deleted successfully' });
  } catch (error) {
    res.status(500).json({ message: 'Internal server error' });
  }
});

// GET /api/server/status
app.get('/api/server/status', authenticateToken, (req, res) => {
  const uptime = Math.floor((Date.now() - serverStartTime) / 1000);

  res.json({
    running: true,
    uptime: uptime,
    nodeId: `node-${Math.random().toString(36).substr(2, 9)}`,
    tls: {
      enabled: true,
      minVersion: 'TLS1.2'
    },
    services: {
      smtp: 'RUNNING',
      imap: 'RUNNING'
    },
    connections: activeConnections
  });
});

// GET /api/server/metrics
app.get('/api/server/metrics', authenticateToken, (req, res) => {
  res.json({
    activeConnections: activeConnections,
    totalConnections: activeConnections + Math.floor(Math.random() * 100),
    uptime: Math.floor((Date.now() - serverStartTime) / 1000),
    memoryUsage: 1024 * 1024 * (50 + Math.floor(Math.random() * 50)), // 50-100MB
    cpuUsage: Math.round((10 + Math.random() * 20) * 10) / 10 // 10-30%
  });
});

// GET /api/logs
app.get('/api/logs', authenticateToken, (req, res) => {
  const limit = parseInt(req.query.limit) || 100;
  const offset = parseInt(req.query.offset) || 0;

  // Generate sample logs
  const logs = [];
  const logLevels = ['info', 'warn', 'error', 'debug'];
  const messages = [
    'Server started successfully',
    'SMTP server listening on port 25',
    'IMAP server listening on port 143',
    'New connection established',
    'Email delivered successfully',
    'Authentication successful',
    'High memory usage detected',
    'Connection timeout',
    'Invalid login attempt',
    'TLS handshake completed'
  ];

  for (let i = offset; i < offset + limit && i < 50; i++) {
    logs.push({
      timestamp: new Date(Date.now() - (i * 60000)).toISOString(),
      level: logLevels[Math.floor(Math.random() * logLevels.length)],
      message: messages[Math.floor(Math.random() * messages.length)]
    });
  }

  res.json({ logs });
});

// Start server
app.listen(PORT, () => {
  console.log(`API Server running on port ${PORT}`);
  console.log(`Frontend URL: http://localhost:5173`);
  console.log(`CORS enabled for frontend origin`);
});