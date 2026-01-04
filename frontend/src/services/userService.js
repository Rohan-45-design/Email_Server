import http from '../api/http';

export const userService = {
  // Get all users
  async getUsers() {
    try {
      const response = await http.get('/users');
      return response.data;
    } catch (error) {
      throw new Error(error.response?.data?.message || 'Failed to fetch users');
    }
  },

  // Create new user
  async createUser(userData) {
    try {
      const response = await http.post('/users', userData);
      return response.data;
    } catch (error) {
      throw new Error(error.response?.data?.message || 'Failed to create user');
    }
  },

  // Update user
  async updateUser(userId, userData) {
    try {
      const response = await http.put(`/users/${userId}`, userData);
      return response.data;
    } catch (error) {
      throw new Error(error.response?.data?.message || 'Failed to update user');
    }
  },

  // Delete user
  async deleteUser(userId) {
    try {
      await http.delete(`/users/${userId}`);
    } catch (error) {
      throw new Error(error.response?.data?.message || 'Failed to delete user');
    }
  },
};