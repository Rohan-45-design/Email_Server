import { useState, useEffect } from 'react';
import { serverService } from '../services/serverService';
import ErrorBanner from '../components/ErrorBanner';
import Loader from '../components/Loader';
import './Dashboard.css';

const Dashboard = () => {
  const [serverStatus, setServerStatus] = useState(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');

  useEffect(() => {
    const fetchServerStatus = async () => {
      try {
        setLoading(true);
        const data = await serverService.getServerStatus();
        setServerStatus(data);
      } catch (err) {
        setError(err.message);
      } finally {
        setLoading(false);
      }
    };

    fetchServerStatus();
  }, []);

  if (loading) return <Loader />;

  return (
    <div className="dashboard">
      <h1>Server Dashboard</h1>
      {error && <ErrorBanner message={error} onClose={() => setError('')} />}

      {serverStatus && (
        <div className="dashboard-grid">
          {/* Server Health Section */}
          <div className="card">
            <h2>Server Health</h2>
            <div className="status-info">
              <p>
                <strong>Status:</strong>
                <span className={`status ${serverStatus.running ? 'running' : 'stopped'}`}>
                  {serverStatus.running ? 'Running' : 'Stopped'}
                </span>
              </p>
              <p><strong>Uptime:</strong> {formatUptime(serverStatus.uptime)}</p>
              <p><strong>Node ID:</strong> {serverStatus.nodeId}</p>
            </div>
          </div>

          {/* Security Section */}
          <div className="card">
            <h2>Security</h2>
            <div className="status-info">
              <p>
                <strong>TLS Enabled:</strong>
                <span className={`status ${serverStatus.tls?.enabled ? 'running' : 'stopped'}`}>
                  {serverStatus.tls?.enabled ? 'Yes' : 'No'}
                </span>
              </p>
              <p><strong>Min TLS Version:</strong> {serverStatus.tls?.minVersion || 'N/A'}</p>
              <p><strong>Auth Enabled:</strong> Yes</p>
            </div>
          </div>

          {/* Mail Services Section */}
          <div className="card">
            <h2>Mail Services</h2>
            <div className="status-info">
              <p>
                <strong>SMTP:</strong>
                <span className={`status ${serverStatus.services?.smtp === 'RUNNING' ? 'running' : 'stopped'}`}>
                  {serverStatus.services?.smtp || 'UNKNOWN'}
                </span>
              </p>
              <p>
                <strong>IMAP:</strong>
                <span className={`status ${serverStatus.services?.imap === 'RUNNING' ? 'running' : 'stopped'}`}>
                  {serverStatus.services?.imap || 'UNKNOWN'}
                </span>
              </p>
              <p><strong>Active Connections:</strong> {serverStatus.connections || 0}</p>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};

// Helper function to format uptime
const formatUptime = (seconds) => {
  if (!seconds) return '0s';

  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const mins = Math.floor((seconds % 3600) / 60);
  const secs = seconds % 60;

  const parts = [];
  if (days > 0) parts.push(`${days}d`);
  if (hours > 0) parts.push(`${hours}h`);
  if (mins > 0) parts.push(`${mins}m`);
  if (secs > 0 && parts.length === 0) parts.push(`${secs}s`);

  return parts.join(' ') || '0s';
};

export default Dashboard;