# Email Server Admin Frontend

A production-grade React frontend for managing a custom email server backend.

## Features

- **Admin Dashboard**: Server status, metrics, and monitoring
- **User Management**: CRUD operations for user accounts
- **Logs Viewer**: Real-time log viewing with pagination
- **Authentication**: Secure login with token-based auth
- **Responsive Design**: Clean, professional UI

## Tech Stack

- **React 18** with Vite
- **React Router** for routing
- **Axios** for HTTP requests
- **CSS Modules** for styling
- **Functional Components + Hooks**

## Project Structure

```
src/
 ├── api/
 │    └── http.js              // Axios instance with interceptors
 ├── services/
 │    ├── authService.js       // Authentication API calls
 │    ├── userService.js       // User management API calls
 │    └── serverService.js     // Server status/metrics API calls
 ├── pages/
 │    ├── Login.jsx
 │    ├── Dashboard.jsx
 │    ├── Users.jsx
 │    ├── Logs.jsx
 │    └── Settings.jsx
 ├── components/
 │    ├── Navbar.jsx
 │    ├── ProtectedRoute.jsx
 │    ├── Loader.jsx
 │    └── ErrorBanner.jsx
 ├── hooks/
 │    └── useAuth.jsx          // Authentication context and hooks
 ├── App.jsx
 └── main.jsx
```

## Getting Started

1. **Install dependencies:**
   ```bash
   npm install
   ```

2. **Configure environment:**
   Create a `.env` file in the root directory:
   ```
   VITE_API_BASE_URL=http://localhost:3000/api
   ```

3. **Start development server:**
   ```bash
   npm run dev
   ```

4. **Build for production:**
   ```bash
   npm run build
   ```

## Backend API Assumptions

The frontend expects the following REST API endpoints:

### Authentication
- `POST /api/auth/login` - Login with credentials
- `GET /api/auth/me` - Get current user info

### Users
- `GET /api/users` - List all users
- `POST /api/users` - Create new user
- `PUT /api/users/:id` - Update user
- `DELETE /api/users/:id` - Delete user

### Server
- `GET /api/server/status` - Server status
- `GET /api/server/metrics` - Server metrics
- `GET /api/logs` - Server logs

## Architecture Principles

- **Separation of Concerns**: API calls in services, UI in components
- **Protected Routes**: Authentication required for admin features
- **Error Handling**: Graceful error handling with user feedback
- **Loading States**: Proper loading indicators
- **Clean Code**: Readable, maintainable, production-ready

## Deployment

The built files in `dist/` can be served by any static web server. Configure the `VITE_API_BASE_URL` environment variable to point to your backend API.
