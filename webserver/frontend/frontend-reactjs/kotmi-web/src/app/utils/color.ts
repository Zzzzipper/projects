export class Color {
    private red: number;
  
    private green: number;
  
    private blue: number;
  
    private alpha: number;
  
    constructor (color: number, alpha = 1) {
      color >>>= 0;
      this.blue = color & 0xFF;
      this.green = (color & 0xFF00) >>> 8;
      this.red = (color & 0xFF0000) >>> 16;
      this.alpha = alpha;
    }
  
    get rgba () {
      return `rgba(${this.red}, ${this.green}, ${this.blue})`;
    }
  }
  