const mongoose = global.db;

export const osmModel = mongoose.model("maps", new mongoose.Schema({}, {timestamps: true, strict: false}));

