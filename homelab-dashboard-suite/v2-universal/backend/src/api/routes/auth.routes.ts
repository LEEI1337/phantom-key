import { Router } from 'express';
import { errors } from '../../middleware/errorHandler';

const router = Router();

// GET /api/auth/login
router.post('/login', async (req, res, next) => {
  try {
    const { username, password } = req.body;
    
    if (!username || !password) {
      throw errors.badRequest('Username and password required');
    }

    // TODO: Implement actual authentication with database
    // For now, return a demo token
    const token = 'demo_jwt_token_' + Date.now();
    
    res.json({
      success: true,
      token,
      user: {
        id: '1',
        username,
        role: 'admin'
      }
    });
  } catch (error) {
    next(error);
  }
});

// GET /api/auth/me
router.get('/me', async (req, res, next) => {
  try {
    // TODO: Validate JWT token from header
    res.json({
      success: true,
      user: {
        id: '1',
        username: 'admin',
        role: 'admin'
      }
    });
  } catch (error) {
    next(error);
  }
});

export { router as authRoutes };
