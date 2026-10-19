import express from "express";
import cors from "cors";
import mongoose from "mongoose";
import dotenv from "dotenv";

import Latest from "./models/latest.js";
import History from "./models/history.js";

dotenv.config();

const app = express();
const PORT = 3000;

const VALID_DEVICE_KEYS = [
    process.env.SECRET_DEVICE_KEY_ONE,
].filter(Boolean);

app.use(cors());
app.use(express.json());

if (VALID_DEVICE_KEYS.length === 0) {
    throw new Error("No SECRET_DEVICE_KEY_* found in env");
}

if (!process.env.MONGODB_URI) {
    throw new Error("MONGODB_URI is missing");
}

app.get("/", (req, res) => {
    res.send("Fire Detection API UP & Running");
});

app.post('/devices/:device_key/api/sensor', async (req, res) => {
    try {
        const { device_id, status } = req.body;

        if (!VALID_DEVICE_KEYS.includes(req.params.device_key)) {
            return res.status(401).json({
                success: false,
                message: "401 - Invalid device key"
            });
        }
        if (device_id == undefined || status === undefined) {
            return res.status(400).json({
                success: false,
                message: "400 - device id and status is required"
            });
        }

        // hehe
        if (device_id !== req.params.device_key) {
            return res.status(403).json({
                success: false,
                message: "403 - Authrization failed."
            });
        }

        if (typeof status !== "number") {
            return res.status(400).json({
                success: false,
                message: "400 - status must be a number"
            });
        }

        if (typeof device_id !== "string") {
            return res.status(400).json({
                success: false,
                message: "400 - device_id must be a string"
            });
        }

        const sensorData = {
            device_id,
            status,
            time: new Date()
        };

        await Latest.findOneAndUpdate(
            { device_id: sensorData.device_id },
            sensorData,
            { upsert: true, new: true }
        );

        await History.create(sensorData);

        res.json({
            success: true,
            message: "Sensor data stored successfully"
        });
    } catch (err) {
        console.error(err.message);
        res.status(500).json({
            success: false,
            message: "500 - Database error"
        });
    }

});

app.get('/devices/:device_key/api/latest', async (req, res) => {
    try {
        if (!VALID_DEVICE_KEYS.includes(req.params.device_key)) {
            return res.status(401).json({
                success: false,
                message: "401 - Invalid device key"
            });
        }

        const latest = await Latest.findOne(
            { device_id: req.params.device_key }
        ).lean();

        res.json({
            success: true,
            data: latest
        });

    } catch (err) {
        console.error(err.message);
        res.status(500).json({
            success: false,
            message: "500 - Database error"
        });
    }

});

app.get('/devices/:device_key/api/history', async (req, res) => {
    try {
        if (!VALID_DEVICE_KEYS.includes(req.params.device_key)) {
            return res.status(401).json({
                success: false,
                message: "401 - Invalid device key"
            });
        }
        const history = await History.find(
            { device_id: req.params.device_key }
        ).sort({ time: 1 }).lean();
        res.json({
            success: true,
            count: history.length,
            data: history
        });
    } catch (err) {
        console.error(err.message);
        res.status(500).json({
            success: false,
            message: "500 - Database error"
        });
    }

});

try {
    if (mongoose.connection.readyState === 0) {
        await mongoose.connect(process.env.MONGODB_URI);
    }

    console.log("✅ Connected to MongoDB");
} catch (err) {
    console.error(err.message);
    process.exit(1);
}

app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});

//yet two improvements can be made:
// 1. Separate device identity from authentication
// 2. Pagination on the history endpoint