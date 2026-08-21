import React from 'react';
import ReactDOM from 'react-dom/client';

function App() {
  return (
    <div style={{ padding: '20px', fontFamily: 'Arial' }}>
      <h1>🚀 Homelab Dashboard Suite v2.0</h1>
      <p>Universal Platform is running!</p>
      <div style={{ marginTop: '20px', padding: '15px', background: '#f0f0f0', borderRadius: '8px' }}>
        <h3>Status:</h3>
        <ul>
          <li>✅ Backend API: http://localhost:3000</li>
          <li>✅ WebSocket: ws://localhost:3000/ws</li>
          <li>⏳ WebUI Editor: Coming in next phase</li>
        </ul>
      </div>
      <p style={{ marginTop: '20px', color: '#666' }}>
        The foundation is ready. Next phases will add the visual editor, multi-device support, and all 30+ integrations.
      </p>
    </div>
  );
}

const root = ReactDOM.createRoot(document.getElementById('root'));
root.render(<App />);
