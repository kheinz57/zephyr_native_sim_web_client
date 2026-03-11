const DEVICE_CONFIG = {
  display: { width: 480, height: 854, depth: 24 },
  frame: {
    enabled: true,
    image: 'caseimage.png',
    width: 837,
    height: 1838
  },
  screen: {
    x: 178,
    y: 254,
    width: 487,
    height: 875
  },
  buttons: [
    // top row
    {id: "M",     key: 0x6d,   scancode: "sc24",  x: 105,  y: 1344, w: 136, h: 112, label:""},
    {id: "H",     key: 0x68,   scancode: "sc19",  x: 350,  y: 1400, w: 130, h: 130, label:""},
    {id: "S",     key: 0x73,   scancode: "sc30",  x: 600,  y: 1344, w: 136, h: 112, label:""},
    {id: "LEFT",  key: 0xff51, scancode: "sc88",  x: 250,  y: 1350, w: 80,  h: 200, label:""},
    {id: "RIGHT", key: 0xff53, scancode: "sc87",  x: 490,  y: 1350, w: 80,  h: 200, label:""},
    {id: "UP",    key: 0xff52, scancode: "sc90",  x: 340,  y: 1290, w: 150, h: 70,  label:""},
    {id: "DOWN",  key: 0xff54, scancode: "sc89",  x: 340,  y: 1560, w: 150, h: 70,  label:""},
   // bottom row
    {id: "T",     key: 0x74,   scancode: "sc31",  x: 135,  y: 1635, w: 126, h: 112, label:""},
    {id: "B",     key: 0x62,   scancode: "sc13",  x: 350,  y: 1635, w: 126, h: 112, label:""},
    {id: "C",     key: 0x63,   scancode: "sc14",  x: 575,  y: 1635, w: 126, h: 112, label:""},
    {id: "F",     key: 0x66,   scancode: "sc17",  x: 612,  y: 1344, w: 136, h: 112, label:""},
  ],
  vnc: { wsPort: 7000 }
};

