import { useState } from 'react';
import './ErrorBanner.css';

const ErrorBanner = ({ message, onClose }) => {
  const [isVisible, setIsVisible] = useState(true);

  const handleClose = () => {
    setIsVisible(false);
    if (onClose) onClose();
  };

  if (!isVisible) return null;

  return (
    <div className="error-banner">
      <span className="error-message">{message}</span>
      <button className="error-close" onClick={handleClose}>
        ×
      </button>
    </div>
  );
};

export default ErrorBanner;