import { Router } from 'express';

const router = Router();

// GET /api/layouts - List all layouts
router.get('/', async (req, res, next) => {
  try {
    res.json({
      success: true,
      data: []
    });
  } catch (error) {
    next(error);
  }
});

export { router as layoutRoutes };
