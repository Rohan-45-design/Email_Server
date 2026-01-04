import http from '../api/http';

export const authService = {
  // Login user
  async login(credentials) {
    try {
      const response = await http.post('/auth/login', credentials);

      // Check if response is successful and contains token
      if (response.status >= 200 && response.status < 300 && response.data.token) {
        const { token } = response.data;
        // Store token in localStorage under "token" key
        localStorage.setItem('token', token);
        return response.data;
      } else {
        throw new Error(response.data?.message || 'Login failed');
      }
    } catch (error) {
      // Handle different types of errors
      if (error.response) {
        // Server responded with error status
        throw new Error(error.response.data?.message || 'Login failed');
      } else if (error.request) {
        // Network error
        throw new Error('Network error - please check your connection');
      } else {
        // Other error
        throw new Error(error.message || 'Login failed');
      }
    }
  },

  // Logout user
  logout() {
    localStorage.removeItem('token');
  },

  // Check if user is authenticated
  isAuthenticated() {
    return !!localStorage.getItem('token');
  },

  // Get current user status
  async getCurrentUser() {
    try {
      const response = await http.get('/auth/me');
      return response.data;
    } catch (error) {
      throw new Error(error.response?.data?.message || 'Failed to get user info');
    }
  },
};