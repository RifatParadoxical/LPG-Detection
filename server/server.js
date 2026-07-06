import express from "express";
import cors from "cors";
import mongoose from "mongoose";
import dotenv from "dotenv";

import Latest from "./models/latest.js";
import History from "./models/history.js";

dotenv.config();

const app = express();
const PORT = 3000;

app.use(cors());
app.use(express.json());

app.get("/", (req, res) => {
    res.send("Fire Detection API Running");
});

app.post("/api/sensor", async (req, res) => {
    try{
            const { ppm, status } = req.body;

    if (ppm === undefined || status === undefined) {
        return res.status(400).json({
            success: false,
            message: "ppm and status are required"
        });
    }

    if (typeof ppm !== "number" || typeof status !== "number") {
        return res.status(400).json({
            success: false,
            message: "ppm and status must be numbers"
        });
    }

    const sensorData = {
        ppm,
        status,
        time: new Date()
    };

    await Latest.findOneAndUpdate(
        {},
        sensorData,
        {
            upsert: true,
            new: true
        }
    );

    const lastHistory = await History.findOne().sort({ time: -1 });

    if ( 
        !lastHistory ||
        sensorData.time - lastHistory.time >= 5 * 60 * 1000
    ) {
        await History.create(sensorData);
    }

    // Keep only last 8640 records, that means last 1 month data
    const count = await History.countDocuments();
    if (count > 8640) {
        const oldest = await History.findOne().sort({ time: 1 });
        await History.findByIdAndDelete(oldest._id);
    }

    res.json({
        success: true,
        message: "Sensor data stored successfully"
    });
    } catch (err) {
        console.error(err.message);
        res.status(500).json({
            success: false,
            message: "Database error"
        });
    }

});

app.get("/api/latest", async (req, res) => {
    try {
        const latest = await Latest.findOne().lean();
        res.json({
            success: true,
            data: latest
        });
    } catch (err) {
        console.error(err.message);
        res.status(500).json({
            success: false,
            message: "Database error"
        });
    }
    
});

app.get("/api/history", async (req, res) => {
    try {
        const history = await History.find().sort({ time: 1 }).lean();
        res.json({
            success: true,
            count: history.length,
            data: history
        });
    } catch (err) {
        console.error(err.message);
        res.status(500).json({
            success: false,
            message: "Database error"
        });
    }

});

try {
    await mongoose.connect(process.env.MONGODB_URI);

    console.log("✅ Connected to MongoDB");
} catch (err) {
    console.error(err.message);
    process.exit(1);
}

app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});