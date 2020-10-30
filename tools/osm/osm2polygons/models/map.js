const mongoose = global.db;

export const mapModel = mongoose.model("maps", new mongoose.Schema({}, {timestamps: true, strict: false}));

