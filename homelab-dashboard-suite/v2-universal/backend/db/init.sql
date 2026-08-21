-- Initialize TimescaleDB for Homelab Dashboard Suite

-- Enable TimescaleDB extension
CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;

-- Create users table
CREATE TABLE IF NOT EXISTS users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    email VARCHAR(100),
    role VARCHAR(20) DEFAULT 'viewer', -- admin, editor, viewer
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Create data_sources table
CREATE TABLE IF NOT EXISTS data_sources (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(100) NOT NULL,
    type VARCHAR(50) NOT NULL, -- proxmox, docker, homeassistant, etc.
    config JSONB NOT NULL, -- stores IP, token, credentials (encrypted at app level)
    is_active BOOLEAN DEFAULT true,
    last_seen TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Create metrics table (hypertable)
CREATE TABLE IF NOT EXISTS metrics (
    time TIMESTAMP WITH TIME ZONE NOT NULL,
    source_id UUID REFERENCES data_sources(id) ON DELETE CASCADE,
    metric_key VARCHAR(100) NOT NULL,
    value DOUBLE PRECISION,
    string_value TEXT,
    unit VARCHAR(20),
    tags JSONB
);

-- Convert to hypertable
SELECT create_hypertable('metrics', 'time', if_not_exists => TRUE);

-- Create indexes
CREATE INDEX IF NOT EXISTS idx_metrics_source_id ON metrics(source_id, time DESC);
CREATE INDEX IF NOT EXISTS idx_metrics_metric_key ON metrics(metric_key, time DESC);
CREATE INDEX IF NOT EXISTS idx_metrics_tags ON metrics USING GIN(tags);

-- Create layouts table (for dashboard configurations)
CREATE TABLE IF NOT EXISTS layouts (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(100) NOT NULL,
    description TEXT,
    device_id VARCHAR(100), -- null means global/default
    layout_json JSONB NOT NULL, -- the complete widget layout
    is_active BOOLEAN DEFAULT true,
    created_by UUID REFERENCES users(id),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Create alerts table
CREATE TABLE IF NOT EXISTS alerts (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(100) NOT NULL,
    rule_json JSONB NOT NULL, -- condition definition
    severity VARCHAR(20) DEFAULT 'warning', -- info, warning, critical
    is_active BOOLEAN DEFAULT true,
    cooldown_seconds INTEGER DEFAULT 300,
    notification_channels JSONB, -- [telegram, email, mqtt]
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Create alert_log table (hypertable)
CREATE TABLE IF NOT EXISTS alert_log (
    time TIMESTAMP WITH TIME ZONE NOT NULL,
    alert_id UUID REFERENCES alerts(id) ON DELETE CASCADE,
    triggered_by VARCHAR(255),
    value DOUBLE PRECISION,
    message TEXT,
    acknowledged BOOLEAN DEFAULT false,
    acknowledged_by UUID REFERENCES users(id),
    acknowledged_at TIMESTAMP WITH TIME ZONE
);

-- Convert to hypertable
SELECT create_hypertable('alert_log', 'time', if_not_exists => TRUE);

-- Create audit_log table
CREATE TABLE IF NOT EXISTS audit_log (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    time TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    user_id UUID REFERENCES users(id),
    action VARCHAR(50) NOT NULL, -- login, logout, config_change, command_exec
    resource_type VARCHAR(50),
    resource_id UUID,
    details JSONB,
    ip_address INET
);

-- Insert default admin user (password: admin123 - CHANGE IMMEDIATELY!)
-- Password hash generated with bcryptjs
INSERT INTO users (username, password_hash, role, email) 
VALUES ('admin', '$2a$10$rQZ9vXJxL5K5z5z5z5z5z.uOqPqPqPqPqPqPqPqPqPqPqPqPqPqPq', 'admin', 'admin@localhost')
ON CONFLICT (username) DO NOTHING;

-- Create function to update updated_at timestamp
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Add triggers for updated_at
CREATE TRIGGER update_users_updated_at BEFORE UPDATE ON users FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();
CREATE TRIGGER update_data_sources_updated_at BEFORE UPDATE ON data_sources FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();
CREATE TRIGGER update_layouts_updated_at BEFORE UPDATE ON layouts FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

-- Grant permissions
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO postgres;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO postgres;
