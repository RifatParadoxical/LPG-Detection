import mongoose from "mongoose";

const historySchema = new mongoose.Schema({
    status: {
        type: Number,
        required: true
    },
    time: {
        type: Date,
        default: Date.now
    }
});

export default mongoose.model("history", historySchema);