import { Router } from 'express';
import { errors } from '../../middleware/errorHandler';

const router = Router();

// GET /api/sources - List all data sources
router.get('/', async (req, res, next) => {
  try {
    // TODO: Fetch from database
    res.json({
      success: true,
      data: []
    });
  } catch (error) {
    next(error);
  }
});

// POST /api/sources - Create new data source
router.post('/', async (req, res, next) => {
  try {
    const { name, type, config } = req.body;
    
    if (!name || !type) {
      throw errors.badRequest('Name and type required');
    }

    // TODO: Save to database
    res.status(201).json({
      success: true,
      data: { id: 'new-id', name, type, config }
    });
  } catch (error) {
    next(error);
  }
});

export { router as dataSourceRoutes };
