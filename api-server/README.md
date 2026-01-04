# Email Server API

REST API backend for the email server admin frontend.

## Quick Start

```bash
npm install
npm start
```

The API server will run on `http://localhost:3000`

## Authentication

### Login
```bash
POST /api/auth/login
Content-Type: application/json

{
  "username": "admin",
  "password": "admin123"
}
```

**Default Credentials:**
- Username: `admin`
- Password: `admin123`

### Using the API
Include the JWT token in the Authorization header:
```
Authorization: Bearer <your-jwt-token>
```

## API Endpoints

### Authentication
- `POST /api/auth/login` - User login
- `GET /api/auth/me` - Get current user info

### User Management
- `GET /api/users` - List all users
- `POST /api/users` - Create new user
- `PUT /api/users/:id` - Update user
- `DELETE /api/users/:id` - Delete user

### Server Status
- `GET /api/server/status` - Server health and status
- `GET /api/server/metrics` - Server performance metrics
- `GET /api/logs?limit=100&offset=0` - Server logs

### Health Check
- `GET /health` - Basic health check (no auth required)

## CORS Configuration

CORS is configured to allow requests from `http://localhost:5175` (the frontend).

## Data Storage

Currently uses in-memory storage. In production, replace with a proper database.

## Environment Variables

- `PORT` - Server port (default: 3000)
- `JWT_SECRET` - JWT signing secret (default: development secret)