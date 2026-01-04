import http from '../api/http';

export const serverService = {
  // Get server status
  async getServerStatus() {
    try {
      const response = await http.get('/server/status');
      return response.data;
    } catch (error) {
      throw new Error(error.response?.data?.message || 'Failed to fetch server status');
    }
  },

  // Get server metrics
  async getServerMetrics() {
    try {
      const response = await http.get('/server/metrics');
      return response.data;
    } catch (error) {
      throw new Error(error.response?.data?.message || 'Failed to fetch server metrics');
    }
  },

  // Get logs
  async getLogs(limit = 100, offset = 0) {
    try {
      const response = await http.get('/logs', {
        params: { limit, offset },
      });
      return response.data;
    } catch (error) {
      throw new Error(error.response?.data?.message || 'Failed to fetch logs');
    }
  },
};