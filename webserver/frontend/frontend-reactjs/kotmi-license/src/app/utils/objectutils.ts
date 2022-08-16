export default class utils {
    public static objKeyValToString = (obj: any): any => {
        if (!obj) return;
        return Object?.keys(obj).map((key) => {
           if (typeof obj[key] == 'string' || typeof obj[key] == 'number') {
              return "" + key + ": " + obj[key];
           } else if (typeof obj[key] == 'object') {
              return "" + key + ': {\n' + this.objKeyValToString(obj[key]) + '\n},';
           }
        }).filter((item) => {
           return item != undefined;
        }).join("\n")
     };
  
     public static findInObject = (obj: any, key: any) => {
        var value: any;
      //   if (!obj) return;
        Object.keys(obj)?.some((k) => {
           if (obj[k] === key) {
              value = obj;
              return true;
           }
           if (obj[k] && typeof obj[k] === 'object') {
              value = this.findInObject(obj[k], key);
              return value !== undefined;
           }
        });
        return value;
     };
}
