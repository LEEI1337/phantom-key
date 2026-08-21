import { Router } from 'express';

const router = Router();

// GET /api/metrics - Get metrics
router.get('/', async (req, res, next) => {
  try {
    res.json({
      success: true,
      data: {
        live: {},
        historical: []
      }
    });
  } catch (error) {
    next(error);
  }
});

export { router as metricsRoutes };
