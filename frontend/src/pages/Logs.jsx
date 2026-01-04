import { useState, useEffect } from 'react';
import { serverService } from '../services/serverService';
import ErrorBanner from '../components/ErrorBanner';
import Loader from '../components/Loader';
import './Logs.css';

const Logs = () => {
  const [logs, setLogs] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [limit, setLimit] = useState(100);
  const [offset, setOffset] = useState(0);

  useEffect(() => {
    fetchLogs();
  }, [limit, offset]);

  const fetchLogs = async () => {
    try {
      setLoading(true);
      const data = await serverService.getLogs(limit, offset);
      setLogs(data.logs || data);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  const handleLimitChange = (e) => {
    setLimit(parseInt(e.target.value));
    setOffset(0); // Reset to first page
  };

  const handlePrevPage = () => {
    if (offset > 0) {
      setOffset(offset - limit);
    }
  };

  const handleNextPage = () => {
    if (logs.length === limit) {
      setOffset(offset + limit);
    }
  };

  const formatTimestamp = (timestamp) => {
    return new Date(timestamp).toLocaleString();
  };

  const getLogLevelClass = (level) => {
    switch (level?.toLowerCase()) {
      case 'error': return 'log-error';
      case 'warn': return 'log-warn';
      case 'info': return 'log-info';
      case 'debug': return 'log-debug';
      default: return 'log-default';
    }
  };

  if (loading) return <Loader />;

  return (
    <div className="logs">
      <div className="logs-header">
        <h1>Server Logs</h1>
        <div className="logs-controls">
          <label>
            Show:
            <select value={limit} onChange={handleLimitChange}>
              <option value={50}>50</option>
              <option value={100}>100</option>
              <option value={200}>200</option>
              <option value={500}>500</option>
            </select>
            entries
          </label>
        </div>
      </div>

      {error && <ErrorBanner message={error} onClose={() => setError('')} />}

      <div className="logs-navigation">
        <button
          onClick={handlePrevPage}
          disabled={offset === 0}
          className="nav-btn"
        >
          Previous
        </button>
        <span>Page {Math.floor(offset / limit) + 1}</span>
        <button
          onClick={handleNextPage}
          disabled={logs.length < limit}
          className="nav-btn"
        >
          Next
        </button>
      </div>

      <div className="logs-container">
        {logs.length === 0 ? (
          <p>No logs found.</p>
        ) : (
          <div className="logs-list">
            {logs.map((log, index) => (
              <div key={index} className={`log-entry ${getLogLevelClass(log.level)}`}>
                <div className="log-meta">
                  <span className="log-timestamp">{formatTimestamp(log.timestamp)}</span>
                  <span className="log-level">{log.level}</span>
                </div>
                <div className="log-message">{log.message}</div>
                {log.details && (
                  <div className="log-details">
                    <pre>{JSON.stringify(log.details, null, 2)}</pre>
                  </div>
                )}
              </div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
};

export default Logs;