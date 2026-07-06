import mongoose from "mongoose";

const latestSchema = new mongoose.Schema({
    ppm: {
        type: Number,
        required: true
    },
    status: {
        type: Number,
        required: true
    },
    time: {
        type: Date,
        default: Date.now
    }
});

export default mongoose.model("latest", latestSchema);