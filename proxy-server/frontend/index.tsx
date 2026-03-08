import {
  type FunctionComponent,
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from "react";
import { createRoot } from "react-dom/client";
import type uPlot from "uplot";
import { movingAverage } from "./moving_average.ts";
import { lowPassFilter, type LowPassFilterSeed } from "./low_pass_filter.ts";
import { UPlotChart } from "./UplotChart.tsx";
import { resolveDefaultApiUrl } from "./resolve_default_api_url.ts";
import { hexToRgba } from "./hex_to_rgba.ts";
import { formatAxisValue } from "./format_axis_value.ts";
import "./index.css";
import "uplot/dist/uPlot.min.css";
import "@picocss/pico";

type ConnectionStatus = "connected" | "connecting" | "disconnected";
type FilterType = "lowpass" | "movavg";
type SmoothingPreset = "light" | "medium" | "heavy" | "custom";

type PerformanceWithMemory = Performance & {
  memory?: {
    usedJSHeapSize: number;
    jsHeapSizeLimit: number;
  };
};

interface PhysicalInput {
  shear_force_N: number;
  vertical_force_N: number;
  shear_displacement_mm: number;
  normal_displacement_mm: number;
  front_vertical_disp_mm: number;
  rear_vertical_disp_mm: number;
  tilt_mm: number;
  shear_stress_kpa: number;
  vertical_stress_kpa: number;
  front_friction_force_N: number;
  rear_friction_force_N: number;
}

interface PhysicalOutput {
  motor_rpm: number;
  front_ep_kpa: number;
  rear_ep_kpa: number;
}

interface SensorData {
  timestamp: number;
  physical_input: PhysicalInput;
  physical_output: PhysicalOutput;
}

interface DataHistory {
  timestamps: number[];
  shearForce: number[];
  verticalForce: number[];
  shearDisp: number[];
  normalDisp: number[];
  frontVerticalDisp: number[];
  rearVerticalDisp: number[];
  tiltMm: number[];
  shearStress: number[];
  verticalStress: number[];
  motorRpm: number[];
  frontEp: number[];
  rearEp: number[];
  frontFriction: number[];
  rearFriction: number[];
}

const createEmptyHistory = (): DataHistory => ({
  timestamps: [],
  shearForce: [],
  verticalForce: [],
  shearDisp: [],
  normalDisp: [],
  frontVerticalDisp: [],
  rearVerticalDisp: [],
  tiltMm: [],
  shearStress: [],
  verticalStress: [],
  motorRpm: [],
  frontEp: [],
  rearEp: [],
  frontFriction: [],
  rearFriction: [],
});

const historyKeys: (keyof DataHistory)[] = [
  "timestamps",
  "shearForce",
  "verticalForce",
  "shearDisp",
  "normalDisp",
  "frontVerticalDisp",
  "rearVerticalDisp",
  "tiltMm",
  "shearStress",
  "verticalStress",
  "motorRpm",
  "frontEp",
  "rearEp",
  "frontFriction",
  "rearFriction",
];

const lowPassCarryKeys = [
  "shearForce",
  "verticalForce",
  "shearDisp",
  "normalDisp",
  "frontVerticalDisp",
  "rearVerticalDisp",
  "tiltMm",
  "shearStress",
  "verticalStress",
  "frontFriction",
  "rearFriction",
  "totalFriction",
] as const;

type LowPassCarryKey = (typeof lowPassCarryKeys)[number];
type LowPassCarryState = Record<LowPassCarryKey, LowPassFilterSeed | null>;

const createEmptyLowPassCarry = (): LowPassCarryState => ({
  shearForce: null,
  verticalForce: null,
  shearDisp: null,
  normalDisp: null,
  frontVerticalDisp: null,
  rearVerticalDisp: null,
  tiltMm: null,
  shearStress: null,
  verticalStress: null,
  frontFriction: null,
  rearFriction: null,
  totalFriction: null,
});

const buildTotalFriction = (
  frontFriction: number[],
  rearFriction: number[],
): number[] => frontFriction.map((front, i) => front + rearFriction[i]);

const updateLowPassCarryFromHistory = (
  history: DataHistory,
  trimStartIdx: number,
  cutoffFrequencyHz: number,
  previousCarry: LowPassCarryState,
): LowPassCarryState => {
  if (
    trimStartIdx <= 0 ||
    history.timestamps.length === 0 ||
    cutoffFrequencyHz <= 0
  ) {
    return previousCarry;
  }

  const lastTrimmedIdx = trimStartIdx - 1;
  const timestamps = history.timestamps;
  const totalFriction = buildTotalFriction(
    history.frontFriction,
    history.rearFriction,
  );
  const seriesMap: Record<LowPassCarryKey, number[]> = {
    shearForce: history.shearForce,
    verticalForce: history.verticalForce,
    shearDisp: history.shearDisp,
    normalDisp: history.normalDisp,
    frontVerticalDisp: history.frontVerticalDisp,
    rearVerticalDisp: history.rearVerticalDisp,
    tiltMm: history.tiltMm,
    shearStress: history.shearStress,
    verticalStress: history.verticalStress,
    frontFriction: history.frontFriction,
    rearFriction: history.rearFriction,
    totalFriction,
  };

  const nextCarry: LowPassCarryState = { ...previousCarry };

  lowPassCarryKeys.forEach((key) => {
    const filteredValues = lowPassFilter(
      seriesMap[key],
      timestamps,
      cutoffFrequencyHz,
      previousCarry[key],
    );
    const filteredValue = filteredValues[lastTrimmedIdx];

    nextCarry[key] = typeof filteredValue === "number"
      ? {
        timestamp: timestamps[lastTrimmedIdx],
        filteredValue,
      }
      : previousCarry[key];
  });

  return nextCarry;
};

const isFilterType = (value: string): value is FilterType => {
  return value === "lowpass" || value === "movavg";
};

const isSmoothingPreset = (value: string): value is SmoothingPreset => {
  return value === "light" || value === "medium" || value === "heavy" ||
    value === "custom";
};

const applyFilter = (
  rawValues: number[],
  timestamps: number[],
  filterType: FilterType,
  filterParam: number,
  lowPassSeed?: LowPassFilterSeed | null,
): number[] => {
  if (filterType === "lowpass") {
    return lowPassFilter(rawValues, timestamps, filterParam, lowPassSeed);
  } else if (filterType === "movavg") {
    return movingAverage(rawValues, timestamps, filterParam);
  }
  return rawValues;
};

const App: FunctionComponent = () => {
  const [status, setStatus] = useState<ConnectionStatus>("disconnected");
  const [error, setError] = useState<string | null>(null);
  const [latestData, setLatestData] = useState<SensorData | null>(null);
  const [dataHistory, setDataHistory] = useState<DataHistory>(
    createEmptyHistory(),
  );
  const [displayWindowSec, setDisplayWindowSec] = useState(60);
  const [apiUrl, setApiUrl] = useState<string>(resolveDefaultApiUrl());
  const [enableSmoothing, setEnableSmoothing] = useState(false);
  const [filterType, setFilterType] = useState<FilterType>("lowpass");
  const [cutoffFrequencyHz, setCutoffFrequencyHz] = useState(2.0);
  const [smoothingWindowMs, setSmoothingWindowMs] = useState(500);
  const [smoothingPreset, setSmoothingPreset] = useState<SmoothingPreset>(
    "medium",
  );

  const eventSourceRef = useRef<EventSource | null>(null);
  const startTimeRef = useRef<number | null>(null);
  const displayWindowSecRef = useRef(60);
  const cutoffFrequencyHzRef = useRef(cutoffFrequencyHz);
  const lowPassCarryRef = useRef<LowPassCarryState>(createEmptyLowPassCarry());
  const lowPassCarryCutoffHzRef = useRef(cutoffFrequencyHz);

  useEffect(() => {
    cutoffFrequencyHzRef.current = cutoffFrequencyHz;
    lowPassCarryRef.current = createEmptyLowPassCarry();
    lowPassCarryCutoffHzRef.current = cutoffFrequencyHz;
  }, [cutoffFrequencyHz]);

  const connectToStream = useCallback(() => {
    if (eventSourceRef.current) eventSourceRef.current.close();
    setStatus("connecting");
    setError(null);
    startTimeRef.current = null;
    lowPassCarryRef.current = createEmptyLowPassCarry();
    lowPassCarryCutoffHzRef.current = cutoffFrequencyHzRef.current;

    try {
      const es = new EventSource(`${apiUrl}/api/sensor-data/stream`);

      es.onopen = () => {
        setStatus("connected");
        console.log("SSE connection established");
      };

      es.addEventListener("data", (event) => {
        try {
          const sensorData = JSON.parse(event.data) as SensorData;
          setLatestData(sensorData);

          setDataHistory((prev) => {
            const newHistory: DataHistory = {
              timestamps: [...prev.timestamps],
              shearForce: [...prev.shearForce],
              verticalForce: [...prev.verticalForce],
              shearDisp: [...prev.shearDisp],
              normalDisp: [...prev.normalDisp],
              frontVerticalDisp: [...prev.frontVerticalDisp],
              rearVerticalDisp: [...prev.rearVerticalDisp],
              tiltMm: [...prev.tiltMm],
              shearStress: [...prev.shearStress],
              verticalStress: [...prev.verticalStress],
              motorRpm: [...prev.motorRpm],
              frontEp: [...prev.frontEp],
              rearEp: [...prev.rearEp],
              frontFriction: [...prev.frontFriction],
              rearFriction: [...prev.rearFriction],
            };
            if (startTimeRef.current === null) {
              startTimeRef.current = sensorData.timestamp;
            }

            const startTime = startTimeRef.current ?? sensorData.timestamp;
            const relativeTime = (sensorData.timestamp - startTime) /
              1000;

            newHistory.timestamps = [...prev.timestamps, relativeTime];
            newHistory.shearForce = [
              ...prev.shearForce,
              sensorData.physical_input.shear_force_N,
            ];
            newHistory.verticalForce = [
              ...prev.verticalForce,
              sensorData.physical_input.vertical_force_N,
            ];
            newHistory.shearDisp = [
              ...prev.shearDisp,
              sensorData.physical_input.shear_displacement_mm,
            ];
            newHistory.normalDisp = [
              ...prev.normalDisp,
              sensorData.physical_input.normal_displacement_mm,
            ];
            newHistory.frontVerticalDisp = [
              ...prev.frontVerticalDisp,
              sensorData.physical_input.front_vertical_disp_mm,
            ];
            newHistory.rearVerticalDisp = [
              ...prev.rearVerticalDisp,
              sensorData.physical_input.rear_vertical_disp_mm,
            ];
            newHistory.tiltMm = [
              ...prev.tiltMm,
              sensorData.physical_input.tilt_mm,
            ];
            newHistory.shearStress = [
              ...prev.shearStress,
              sensorData.physical_input.shear_stress_kpa,
            ];
            newHistory.verticalStress = [
              ...prev.verticalStress,
              sensorData.physical_input.vertical_stress_kpa,
            ];
            newHistory.motorRpm = [
              ...prev.motorRpm,
              sensorData.physical_output.motor_rpm,
            ];
            newHistory.frontEp = [
              ...prev.frontEp,
              sensorData.physical_output.front_ep_kpa,
            ];
            newHistory.rearEp = [
              ...prev.rearEp,
              sensorData.physical_output.rear_ep_kpa,
            ];
            newHistory.frontFriction = [
              ...prev.frontFriction,
              sensorData.physical_input.front_friction_force_N,
            ];
            newHistory.rearFriction = [
              ...prev.rearFriction,
              sensorData.physical_input.rear_friction_force_N,
            ];

            const timeWindow = Math.max(displayWindowSecRef.current, 1);
            const cutoffTime = relativeTime - timeWindow;

            if (
              newHistory.timestamps.length > 0 &&
              newHistory.timestamps[0] < cutoffTime
            ) {
              const startIdx = newHistory.timestamps.findIndex((t) =>
                t >= cutoffTime
              );
              if (startIdx >= 0) {
                if (startIdx > 0) {
                  lowPassCarryRef.current = updateLowPassCarryFromHistory(
                    newHistory,
                    startIdx,
                    cutoffFrequencyHzRef.current,
                    lowPassCarryRef.current,
                  );
                  lowPassCarryCutoffHzRef.current =
                    cutoffFrequencyHzRef.current;
                }

                historyKeys.forEach((key) => {
                  newHistory[key] = newHistory[key].slice(startIdx);
                });
              }
            }

            const perf = globalThis.performance as
              | PerformanceWithMemory
              | undefined;
            if (
              perf?.memory &&
              Math.random() < 0.01
            ) {
              console.debug("Memory:", {
                usedJSHeapSize:
                  (perf.memory.usedJSHeapSize / 1048576).toFixed(2) +
                  " MB",
                jsHeapSizeLimit:
                  (perf.memory.jsHeapSizeLimit / 1048576).toFixed(2) +
                  " MB",
                historyLength: newHistory.timestamps.length,
              });
            }

            return newHistory;
          });
        } catch (err) {
          console.error("Failed to parse sensor data:", err);
        }
      });

      es.onerror = () => {
        console.error("SSE error");
        setStatus("disconnected");
        setError("Connection lost. Please check if the server is running.");
        es.close();
        eventSourceRef.current = null;
      };

      eventSourceRef.current = es;
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      setError(`Failed to connect: ${message}`);
      setStatus("disconnected");
    }
  }, [apiUrl]);

  const disconnect = useCallback(() => {
    if (eventSourceRef.current) {
      eventSourceRef.current.close();
      eventSourceRef.current = null;
    }
    setStatus("disconnected");
  }, []);

  const clearData = useCallback(() => {
    startTimeRef.current = null;
    lowPassCarryRef.current = createEmptyLowPassCarry();
    lowPassCarryCutoffHzRef.current = cutoffFrequencyHzRef.current;
    setDataHistory(createEmptyHistory());
  }, []);

  useEffect(() => {
    return () => {
      if (eventSourceRef.current) eventSourceRef.current.close();
    };
  }, []);

  const getStatusText = () => {
    const map: Record<ConnectionStatus, string> = {
      connected: "Connected",
      connecting: "Connecting...",
      disconnected: "Disconnected",
    };
    return map[status];
  };

  const filterParam = filterType === "lowpass"
    ? cutoffFrequencyHz
    : smoothingWindowMs;

  const lowPassCarry = lowPassCarryCutoffHzRef.current === cutoffFrequencyHz
    ? lowPassCarryRef.current
    : null;

  const totalFriction = useMemo(
    () =>
      dataHistory.frontFriction.length > 0
        ? buildTotalFriction(
          dataHistory.frontFriction,
          dataHistory.rearFriction,
        )
        : [],
    [dataHistory.frontFriction, dataHistory.rearFriction],
  );

  const filteredShearForce = useMemo(
    () =>
      enableSmoothing && filterParam > 0 && dataHistory.shearForce.length > 0
        ? applyFilter(
          dataHistory.shearForce,
          dataHistory.timestamps,
          filterType,
          filterParam,
          lowPassCarry?.shearForce,
        )
        : dataHistory.shearForce,
    [
      dataHistory.shearForce,
      dataHistory.timestamps,
      enableSmoothing,
      filterType,
      filterParam,
    ],
  );

  const filteredFrontFriction = useMemo(
    () =>
      enableSmoothing && filterParam > 0 && dataHistory.frontFriction.length > 0
        ? applyFilter(
          dataHistory.frontFriction,
          dataHistory.timestamps,
          filterType,
          filterParam,
          lowPassCarry?.frontFriction,
        )
        : dataHistory.frontFriction,
    [
      dataHistory.frontFriction,
      dataHistory.timestamps,
      enableSmoothing,
      filterType,
      filterParam,
    ],
  );

  const filteredRearFriction = useMemo(
    () =>
      enableSmoothing && filterParam > 0 && dataHistory.rearFriction.length > 0
        ? applyFilter(
          dataHistory.rearFriction,
          dataHistory.timestamps,
          filterType,
          filterParam,
          lowPassCarry?.rearFriction,
        )
        : dataHistory.rearFriction,
    [
      dataHistory.rearFriction,
      dataHistory.timestamps,
      enableSmoothing,
      filterType,
      filterParam,
    ],
  );

  const filteredTotalFriction = useMemo(
    () =>
      enableSmoothing && filterParam > 0 && totalFriction.length > 0
        ? applyFilter(
          totalFriction,
          dataHistory.timestamps,
          filterType,
          filterParam,
          lowPassCarry?.totalFriction,
        )
        : totalFriction,
    [
      totalFriction,
      dataHistory.timestamps,
      enableSmoothing,
      filterType,
      filterParam,
    ],
  );

  const filteredVerticalForce = useMemo(
    () =>
      enableSmoothing && filterParam > 0 && dataHistory.verticalForce.length > 0
        ? applyFilter(
          dataHistory.verticalForce,
          dataHistory.timestamps,
          filterType,
          filterParam,
          lowPassCarry?.verticalForce,
        )
        : dataHistory.verticalForce,
    [
      dataHistory.verticalForce,
      dataHistory.timestamps,
      enableSmoothing,
      filterType,
      filterParam,
    ],
  );

  const filteredVerticalStress = useMemo(
    () =>
      enableSmoothing && filterParam > 0 &&
        dataHistory.verticalStress.length > 0
        ? applyFilter(
          dataHistory.verticalStress,
          dataHistory.timestamps,
          filterType,
          filterParam,
          lowPassCarry?.verticalStress,
        )
        : dataHistory.verticalStress,
    [
      dataHistory.verticalStress,
      dataHistory.timestamps,
      enableSmoothing,
      filterType,
      filterParam,
    ],
  );

  const filteredNormalDisp = useMemo(
    () =>
      enableSmoothing && filterParam > 0 && dataHistory.normalDisp.length > 0
        ? applyFilter(
          dataHistory.normalDisp,
          dataHistory.timestamps,
          filterType,
          filterParam,
          lowPassCarry?.normalDisp,
        )
        : dataHistory.normalDisp,
    [
      dataHistory.normalDisp,
      dataHistory.timestamps,
      enableSmoothing,
      filterType,
      filterParam,
    ],
  );

  const filteredShearStress = useMemo(
    () =>
      enableSmoothing && filterParam > 0 && dataHistory.shearStress.length > 0
        ? applyFilter(
          dataHistory.shearStress,
          dataHistory.timestamps,
          filterType,
          filterParam,
          lowPassCarry?.shearStress,
        )
        : dataHistory.shearStress,
    [
      dataHistory.shearStress,
      dataHistory.timestamps,
      enableSmoothing,
      filterType,
      filterParam,
    ],
  );

  const filteredShearDisp = useMemo(
    () =>
      enableSmoothing && filterParam > 0 && dataHistory.shearDisp.length > 0
        ? applyFilter(
          dataHistory.shearDisp,
          dataHistory.timestamps,
          filterType,
          filterParam,
          lowPassCarry?.shearDisp,
        )
        : dataHistory.shearDisp,
    [
      dataHistory.shearDisp,
      dataHistory.timestamps,
      enableSmoothing,
      filterType,
      filterParam,
    ],
  );

  const filteredFrontVerticalDisp = useMemo(
    () =>
      enableSmoothing && filterParam > 0 &&
        dataHistory.frontVerticalDisp.length > 0
        ? applyFilter(
          dataHistory.frontVerticalDisp,
          dataHistory.timestamps,
          filterType,
          filterParam,
          lowPassCarry?.frontVerticalDisp,
        )
        : dataHistory.frontVerticalDisp,
    [
      dataHistory.frontVerticalDisp,
      dataHistory.timestamps,
      enableSmoothing,
      filterType,
      filterParam,
    ],
  );

  const filteredRearVerticalDisp = useMemo(
    () =>
      enableSmoothing && filterParam > 0 &&
        dataHistory.rearVerticalDisp.length > 0
        ? applyFilter(
          dataHistory.rearVerticalDisp,
          dataHistory.timestamps,
          filterType,
          filterParam,
          lowPassCarry?.rearVerticalDisp,
        )
        : dataHistory.rearVerticalDisp,
    [
      dataHistory.rearVerticalDisp,
      dataHistory.timestamps,
      enableSmoothing,
      filterType,
      filterParam,
    ],
  );

  const filteredTiltMm = useMemo(
    () =>
      enableSmoothing && filterParam > 0 && dataHistory.tiltMm.length > 0
        ? applyFilter(
          dataHistory.tiltMm,
          dataHistory.timestamps,
          filterType,
          filterParam,
          lowPassCarry?.tiltMm,
        )
        : dataHistory.tiltMm,
    [
      dataHistory.tiltMm,
      dataHistory.timestamps,
      enableSmoothing,
      filterType,
      filterParam,
    ],
  );

  const shearForceData = useMemo(
    () =>
      enableSmoothing && filterParam > 0
        ? [
          dataHistory.timestamps,
          dataHistory.shearForce,
          dataHistory.frontFriction,
          dataHistory.rearFriction,
          totalFriction,
          filteredShearForce,
          filteredFrontFriction,
          filteredRearFriction,
          filteredTotalFriction,
        ]
        : [
          dataHistory.timestamps,
          dataHistory.shearForce,
          dataHistory.frontFriction,
          dataHistory.rearFriction,
          totalFriction,
        ],
    [
      dataHistory.timestamps,
      dataHistory.shearForce,
      dataHistory.frontFriction,
      dataHistory.rearFriction,
      totalFriction,
      filteredShearForce,
      filteredFrontFriction,
      filteredRearFriction,
      filteredTotalFriction,
      enableSmoothing,
      filterParam,
    ],
  );

  const verticalForceData = useMemo(
    () =>
      enableSmoothing && filterParam > 0
        ? [
          dataHistory.timestamps,
          dataHistory.verticalForce,
          filteredVerticalForce,
        ]
        : [dataHistory.timestamps, dataHistory.verticalForce],
    [
      dataHistory.timestamps,
      dataHistory.verticalForce,
      filteredVerticalForce,
      enableSmoothing,
      filterParam,
    ],
  );

  const verticalStressData: number[][] = useMemo(
    () =>
      enableSmoothing && filterParam > 0
        ? [
          dataHistory.timestamps,
          dataHistory.verticalStress,
          dataHistory.normalDisp,
          filteredVerticalStress,
          filteredNormalDisp,
        ]
        : [
          dataHistory.timestamps,
          dataHistory.verticalStress,
          dataHistory.normalDisp,
        ],
    [
      dataHistory.timestamps,
      dataHistory.verticalStress,
      dataHistory.normalDisp,
      filteredVerticalStress,
      filteredNormalDisp,
      enableSmoothing,
      filterParam,
    ],
  );

  const shearStressData: number[][] = useMemo(
    () =>
      enableSmoothing && filterParam > 0
        ? [
          dataHistory.timestamps,
          dataHistory.shearStress,
          dataHistory.shearDisp,
          filteredShearStress,
          filteredShearDisp,
        ]
        : [
          dataHistory.timestamps,
          dataHistory.shearStress,
          dataHistory.shearDisp,
        ],
    [
      dataHistory.timestamps,
      dataHistory.shearStress,
      dataHistory.shearDisp,
      filteredShearStress,
      filteredShearDisp,
      enableSmoothing,
      filterParam,
    ],
  );

  const verticalDispTiltData: number[][] = useMemo(
    () =>
      enableSmoothing && filterParam > 0
        ? [
          dataHistory.timestamps,
          dataHistory.frontVerticalDisp,
          dataHistory.rearVerticalDisp,
          dataHistory.tiltMm,
          filteredFrontVerticalDisp,
          filteredRearVerticalDisp,
          filteredTiltMm,
        ]
        : [
          dataHistory.timestamps,
          dataHistory.frontVerticalDisp,
          dataHistory.rearVerticalDisp,
          dataHistory.tiltMm,
        ],
    [
      dataHistory.timestamps,
      dataHistory.frontVerticalDisp,
      dataHistory.rearVerticalDisp,
      dataHistory.tiltMm,
      filteredFrontVerticalDisp,
      filteredRearVerticalDisp,
      filteredTiltMm,
      enableSmoothing,
      filterParam,
    ],
  );

  const epData: number[][] = useMemo(
    () => [dataHistory.timestamps, dataHistory.frontEp, dataHistory.rearEp],
    [dataHistory.timestamps, dataHistory.frontEp, dataHistory.rearEp],
  );

  const rpmData: number[][] = useMemo(
    () => [dataHistory.timestamps, dataHistory.motorRpm],
    [dataHistory.timestamps, dataHistory.motorRpm],
  );

  const palette = useMemo(() => ({
    shear: "#2563eb",
    shearAlt: "#38bdf8",
    vertical: "#dc2626",
    verticalAlt: "#f97316",
    verticalFront: "#b91c1c",
    verticalRear: "#fb7185",
    tilt: "#94a3b8",
    epFront: "#0f766e",
    epRear: "#14b8a6",
    rpm: "#06b6d4",
    shearFront: "#1d4ed8",
    shearRear: "#60a5fa",
    frictionFront: "#7c3aed",
    frictionRear: "#a78bfa",
    frictionTotal: "#8b5cf6",
  }), []);

  const axisTheme = useMemo(() => ({
    text: getComputedStyle(document.documentElement).getPropertyValue(
      "--muted-color",
    ).trim() || "#e5e7eb",
    grid: getComputedStyle(document.documentElement).getPropertyValue(
      "--card-border-color",
    ).trim() || "#4b5563",
    font: '12px "Segoe UI", sans-serif',
  }), []);

  const shearForceOptions = useMemo<Omit<uPlot.Options, "width" | "height">>(
    () => ({
      series: [
        { label: "Time (s)", scale: "x" },
        {
          label: "Shear Force (N)",
          stroke: enableSmoothing
            ? hexToRgba(palette.shear, 0.3)
            : palette.shear,
          width: 1.5,
          scale: "force",
        },
        {
          label: "Front Friction (N)",
          stroke: enableSmoothing
            ? hexToRgba(palette.frictionFront, 0.3)
            : palette.frictionFront,
          width: 1.5,
          scale: "friction",
          show: false,
        },
        {
          label: "Rear Friction (N)",
          stroke: enableSmoothing
            ? hexToRgba(palette.frictionRear, 0.3)
            : palette.frictionRear,
          width: 1.5,
          scale: "friction",
          show: false,
        },
        {
          label: "Total Friction (N)",
          stroke: enableSmoothing
            ? hexToRgba(palette.frictionTotal, 0.3)
            : palette.frictionTotal,
          width: 1.5,
          scale: "friction",
        },
        ...(enableSmoothing
          ? [
            {
              label: "Shear Smoothed",
              stroke: palette.shear,
              width: 1.0,
              scale: "force",
            },
            {
              label: "Front Friction Smoothed",
              stroke: palette.frictionFront,
              width: 1.0,
              scale: "friction",
              show: false,
            },
            {
              label: "Rear Friction Smoothed",
              stroke: palette.frictionRear,
              width: 1.0,
              scale: "friction",
              show: false,
            },
            {
              label: "Total Friction Smoothed",
              stroke: palette.frictionTotal,
              width: 1.0,
              scale: "friction",
            },
          ]
          : []),
      ],
      axes: [
        {
          stroke: axisTheme.text,
          font: axisTheme.font,
          values: formatAxisValue,
          grid: { stroke: axisTheme.grid, width: 1 },
          ticks: { stroke: axisTheme.text, width: 1 },
        },
        {
          scale: "force",
          label: "Shear Force (N)",
          labelSize: 30,
          stroke: axisTheme.text,
          font: axisTheme.font,
          labelFont: axisTheme.font,
          labelStroke: axisTheme.text,
          values: formatAxisValue,
          grid: { stroke: axisTheme.grid, width: 1 },
          ticks: { stroke: axisTheme.text, width: 1 },
        },
        {
          scale: "friction",
          label: "Friction (N)",
          labelSize: 30,
          side: 1,
          stroke: axisTheme.text,
          font: axisTheme.font,
          labelFont: axisTheme.font,
          labelStroke: axisTheme.text,
          values: formatAxisValue,
          grid: { stroke: axisTheme.grid, width: 1 },
          ticks: { stroke: axisTheme.text, width: 1 },
        },
      ],
      scales: { x: { time: false }, force: {}, friction: {} },
    }),
    [axisTheme, palette, enableSmoothing],
  );

  const verticalForceOptions = useMemo<Omit<uPlot.Options, "width" | "height">>(
    () => ({
      series: [
        { label: "Time (s)" },
        {
          label: "Vertical Force (N)",
          stroke: enableSmoothing
            ? hexToRgba(palette.vertical, 0.3)
            : palette.vertical,
          width: 1.5,
        },
        ...(enableSmoothing
          ? [{ label: "Smoothed", stroke: palette.vertical, width: 1.0 }]
          : []),
      ],
      axes: [
        {
          stroke: axisTheme.text,
          font: axisTheme.font,
          values: formatAxisValue,
          grid: { stroke: axisTheme.grid, width: 1 },
          ticks: { stroke: axisTheme.text, width: 1 },
        },
        {
          label: "Force (N)",
          labelSize: 30,
          stroke: axisTheme.text,
          font: axisTheme.font,
          labelFont: axisTheme.font,
          labelStroke: axisTheme.text,
          values: formatAxisValue,
          grid: { stroke: axisTheme.grid, width: 1 },
          ticks: { stroke: axisTheme.text, width: 1 },
        },
      ],
      scales: { x: { time: false }, y: {} },
    }),
    [axisTheme, palette, enableSmoothing],
  );

  const verticalStressOptions = useMemo<
    Omit<uPlot.Options, "width" | "height">
  >(() => ({
    series: [
      { label: "Time (s)", scale: "x" },
      {
        label: "Vertical Stress (kPa)",
        stroke: enableSmoothing
          ? hexToRgba(palette.vertical, 0.3)
          : palette.vertical,
        width: 1.5,
        scale: "stress",
      },
      {
        label: "Normal Disp. (mm)",
        stroke: enableSmoothing
          ? hexToRgba(palette.verticalAlt, 0.3)
          : palette.verticalAlt,
        width: 1.5,
        scale: "disp",
      },
      ...(enableSmoothing
        ? [
          {
            label: "Stress Smoothed",
            stroke: palette.vertical,
            width: 1.0,
            scale: "stress",
          },
          {
            label: "Disp Smoothed",
            stroke: palette.verticalAlt,
            width: 1.0,
            scale: "disp",
          },
        ]
        : []),
    ],
    axes: [
      {
        stroke: axisTheme.text,
        font: axisTheme.font,
        values: formatAxisValue,
        grid: { stroke: axisTheme.grid, width: 1 },
        ticks: { stroke: axisTheme.text, width: 1 },
      },
      {
        scale: "stress",
        label: "Stress (kPa)",
        labelSize: 30,
        stroke: axisTheme.text,
        font: axisTheme.font,
        labelFont: axisTheme.font,
        labelStroke: axisTheme.text,
        values: formatAxisValue,
        grid: { stroke: axisTheme.grid, width: 1 },
        ticks: { stroke: axisTheme.text, width: 1 },
      },
      {
        scale: "disp",
        label: "Disp (mm)",
        labelSize: 30,
        side: 1,
        stroke: axisTheme.text,
        font: axisTheme.font,
        labelFont: axisTheme.font,
        labelStroke: axisTheme.text,
        values: formatAxisValue,
        grid: { stroke: axisTheme.grid, width: 1 },
        ticks: { stroke: axisTheme.text, width: 1 },
      },
    ],
    scales: { x: { time: false }, stress: {}, disp: {} },
  }), [axisTheme, palette, enableSmoothing]);

  const shearStressOptions = useMemo<Omit<uPlot.Options, "width" | "height">>(
    () => ({
      series: [
        { label: "Time (s)", scale: "x" },
        {
          label: "Shear Stress (kPa)",
          stroke: enableSmoothing
            ? hexToRgba(palette.shear, 0.3)
            : palette.shear,
          width: 1.5,
          scale: "stress",
        },
        {
          label: "Shear Disp. (mm)",
          stroke: enableSmoothing
            ? hexToRgba(palette.shearAlt, 0.3)
            : palette.shearAlt,
          width: 1.5,
          scale: "disp",
        },
        ...(enableSmoothing
          ? [
            {
              label: "Stress Smoothed",
              stroke: palette.shear,
              width: 1.0,
              scale: "stress",
            },
            {
              label: "Disp Smoothed",
              stroke: palette.shearAlt,
              width: 1.0,
              scale: "disp",
            },
          ]
          : []),
      ],
      axes: [
        {
          stroke: axisTheme.text,
          font: axisTheme.font,
          values: formatAxisValue,
          grid: { stroke: axisTheme.grid, width: 1 },
          ticks: { stroke: axisTheme.text, width: 1 },
        },
        {
          scale: "stress",
          label: "Stress (kPa)",
          labelSize: 30,
          stroke: axisTheme.text,
          font: axisTheme.font,
          labelFont: axisTheme.font,
          labelStroke: axisTheme.text,
          values: formatAxisValue,
          grid: { stroke: axisTheme.grid, width: 1 },
          ticks: { stroke: axisTheme.text, width: 1 },
        },
        {
          scale: "disp",
          label: "Disp (mm)",
          labelSize: 30,
          side: 1,
          stroke: axisTheme.text,
          font: axisTheme.font,
          labelFont: axisTheme.font,
          labelStroke: axisTheme.text,
          values: formatAxisValue,
          grid: { stroke: axisTheme.grid, width: 1 },
          ticks: { stroke: axisTheme.text, width: 1 },
        },
      ],
      scales: { x: { time: false }, stress: {}, disp: {} },
    }),
    [axisTheme, palette, enableSmoothing],
  );

  const verticalDispTiltOptions = useMemo<
    Omit<uPlot.Options, "width" | "height">
  >(() => ({
    series: [
      { label: "Time (s)", scale: "x" },
      {
        label: "Front Disp (mm)",
        stroke: enableSmoothing
          ? hexToRgba(palette.verticalFront, 0.3)
          : palette.verticalFront,
        width: 1.5,
        scale: "disp",
        show: false,
      },
      {
        label: "Rear Disp (mm)",
        stroke: enableSmoothing
          ? hexToRgba(palette.verticalRear, 0.3)
          : palette.verticalRear,
        width: 1.5,
        scale: "disp",
        show: false,
      },
      {
        label: "Tilt (mm)",
        stroke: enableSmoothing ? hexToRgba(palette.tilt, 0.3) : palette.tilt,
        width: 1.5,
        scale: "tilt",
      },
      ...(enableSmoothing
        ? [
          {
            label: "Front Smoothed",
            stroke: palette.verticalFront,
            width: 1.0,
            scale: "disp",
            show: false,
          },
          {
            label: "Rear Smoothed",
            stroke: palette.verticalRear,
            width: 1.0,
            scale: "disp",
            show: false,
          },
          {
            label: "Tilt Smoothed",
            stroke: palette.tilt,
            width: 1.0,
            scale: "tilt",
          },
        ]
        : []),
    ],
    axes: [
      {
        stroke: axisTheme.text,
        font: axisTheme.font,
        values: formatAxisValue,
        grid: { stroke: axisTheme.grid, width: 1 },
        ticks: { stroke: axisTheme.text, width: 1 },
      },
      {
        scale: "disp",
        label: "Disp (mm)",
        labelSize: 30,
        stroke: axisTheme.text,
        font: axisTheme.font,
        labelFont: axisTheme.font,
        labelStroke: axisTheme.text,
        values: formatAxisValue,
        grid: { stroke: axisTheme.grid, width: 1 },
        ticks: { stroke: axisTheme.text, width: 1 },
      },
      {
        scale: "tilt",
        label: "Tilt (mm)",
        labelSize: 30,
        side: 1,
        stroke: axisTheme.text,
        font: axisTheme.font,
        labelFont: axisTheme.font,
        labelStroke: axisTheme.text,
        values: formatAxisValue,
        grid: { stroke: axisTheme.grid, width: 1 },
        ticks: { stroke: axisTheme.text, width: 1 },
      },
    ],
    scales: { x: { time: false }, disp: {}, tilt: {} },
  }), [axisTheme, palette, enableSmoothing]);

  const epOptions = useMemo<Omit<uPlot.Options, "width" | "height">>(() => ({
    series: [
      { label: "Time (s)" },
      { label: "Front EP (kPa)", stroke: palette.epFront, width: 1.5 },
      { label: "Rear EP (kPa)", stroke: palette.epRear, width: 1.5 },
    ],
    axes: [
      {
        stroke: axisTheme.text,
        font: axisTheme.font,
        grid: { stroke: axisTheme.grid, width: 1 },
        ticks: { stroke: axisTheme.text, width: 1 },
      },
      {
        label: "EP (kPa)",
        labelSize: 30,
        stroke: axisTheme.text,
        font: axisTheme.font,
        labelFont: axisTheme.font,
        labelStroke: axisTheme.text,
        grid: { stroke: axisTheme.grid, width: 1 },
        ticks: { stroke: axisTheme.text, width: 1 },
      },
    ],
    scales: { x: { time: false }, y: {} },
  }), [axisTheme, palette]);

  const rpmOptions = useMemo<Omit<uPlot.Options, "width" | "height">>(() => ({
    series: [
      { label: "Time (s)" },
      { label: "Motor RPM", stroke: palette.rpm, width: 1.5 },
    ],
    axes: [
      {
        stroke: axisTheme.text,
        font: axisTheme.font,
        grid: { stroke: axisTheme.grid, width: 1 },
        ticks: { stroke: axisTheme.text, width: 1 },
      },
      {
        label: "RPM",
        labelSize: 30,
        stroke: axisTheme.text,
        font: axisTheme.font,
        labelFont: axisTheme.font,
        labelStroke: axisTheme.text,
        grid: { stroke: axisTheme.grid, width: 1 },
        ticks: { stroke: axisTheme.text, width: 1 },
      },
    ],
    scales: { x: { time: false }, y: {} },
  }), [axisTheme, palette]);

  return (
    <main className="container">
      <div className="header">
        <h1>DigitShowDST Sensor Data Viewer</h1>
        <div className="status">
          <div className={"status-indicator " + status}></div>
          <span>{getStatusText()}</span>
        </div>
      </div>

      {error
        ? (
          <div className="error-message">
            <strong>Error:</strong> {error}
          </div>
        )
        : null}

      <div className="controls">
        <input
          type="text"
          value={apiUrl}
          onChange={(e) => setApiUrl(e.target.value)}
          placeholder="API URL"
          style={{ maxWidth: "300px" }}
        />
        <button
          type="button"
          onClick={connectToStream}
          disabled={status === "connected"}
        >
          Connect
        </button>
        <button
          type="button"
          onClick={disconnect}
          disabled={status === "disconnected"}
        >
          Disconnect
        </button>
        <button type="button" onClick={clearData}>Clear Data</button>
        <label>
          Display Window (s):{" "}
          <input
            type="number"
            value={displayWindowSec}
            onChange={(e) => {
              const next = Math.max(5, parseInt(e.target.value, 10) || 60);
              displayWindowSecRef.current = next;
              setDisplayWindowSec(next);
            }}
            min="5"
            max="600"
            step="5"
            style={{ width: "110px" }}
          />
        </label>
      </div>

      <div className="controls">
        <label>
          <input
            type="checkbox"
            checked={enableSmoothing}
            onChange={(e) => setEnableSmoothing(e.target.checked)}
          />{" "}
          Smoothing
        </label>
        {enableSmoothing &&
          (
            <>
              <select
                key="filterType"
                value={filterType}
                onChange={(e) => {
                  const nextType = e.target.value;
                  if (isFilterType(nextType)) {
                    setFilterType(nextType);
                  }
                }}
                style={{ width: "auto", minWidth: "150px" }}
              >
                <option key="lowpass" value="lowpass">Low-Pass Filter</option>
                <option key="movavg" value="movavg">Moving Average</option>
              </select>
              {filterType === "lowpass"
                ? (
                  <>
                    <select
                      key="preset"
                      value={smoothingPreset}
                      onChange={(e) => {
                        const val = e.target.value;
                        if (!isSmoothingPreset(val)) return;
                        setSmoothingPreset(val);
                        const presets: Record<
                          Exclude<SmoothingPreset, "custom">,
                          number
                        > = { light: 1.0, medium: 0.5, heavy: 0.1 };
                        if (val !== "custom") {
                          setCutoffFrequencyHz(presets[val]);
                        }
                      }}
                      style={{ width: "auto", minWidth: "150px" }}
                    >
                      <option key="light" value="light">Light (1 Hz)</option>
                      <option key="medium" value="medium">
                        Medium (0.5 Hz)
                      </option>
                      <option key="heavy" value="heavy">Heavy (0.1 Hz)</option>
                      <option key="custom" value="custom">Custom</option>
                    </select>
                    {smoothingPreset === "custom"
                      ? (
                        <label key="cutoff">
                          Cutoff (Hz):{" "}
                          <input
                            type="number"
                            value={cutoffFrequencyHz}
                            onChange={(e) => {
                              const val = Math.max(
                                0.1,
                                Math.min(50, parseFloat(e.target.value) || 2.0),
                              );
                              setCutoffFrequencyHz(val);
                              setSmoothingPreset("custom");
                            }}
                            min="0.1"
                            max="50"
                            step="0.1"
                            style={{ width: "auto", minWidth: "80px" }}
                          />
                        </label>
                      )
                      : null}
                  </>
                )
                : (
                  <>
                    <select
                      key="preset"
                      value={smoothingPreset}
                      onChange={(e) => {
                        const val = e.target.value;
                        if (!isSmoothingPreset(val)) return;
                        setSmoothingPreset(val);
                        const presets = {
                          light: 200,
                          medium: 500,
                          heavy: 1000,
                        } as const;
                        if (val !== "custom") {
                          setSmoothingWindowMs(presets[val]);
                        }
                      }}
                      style={{ width: "auto", minWidth: "150px" }}
                    >
                      <option key="light" value="light">Light (200ms)</option>
                      <option key="medium" value="medium">
                        Medium (500ms)
                      </option>
                      <option key="heavy" value="heavy">Heavy (1000ms)</option>
                      <option key="custom" value="custom">Custom</option>
                    </select>
                    {smoothingPreset === "custom"
                      ? (
                        <label key="window">
                          Window (ms):{" "}
                          <input
                            type="number"
                            value={smoothingWindowMs}
                            onChange={(e) => {
                              const val = Math.max(
                                50,
                                Math.min(
                                  60 * 1000,
                                  parseInt(e.target.value, 10) || 500,
                                ),
                              );
                              setSmoothingWindowMs(val);
                              setSmoothingPreset("custom");
                            }}
                            min="50"
                            max="60000"
                            step="50"
                            style={{ width: "auto", minWidth: "80px" }}
                          />
                        </label>
                      )
                      : null}
                  </>
                )}
            </>
          )}
      </div>

      {latestData &&
        (
          <div className="metrics">
            <div className="metric-card">
              <p className="metric-label">Shear Force</p>
              <p className="metric-value">
                {latestData.physical_input.shear_force_N.toFixed(2)} N
              </p>
            </div>
            <div className="metric-card">
              <p className="metric-label">Vertical Force</p>
              <p className="metric-value">
                {latestData.physical_input.vertical_force_N.toFixed(2)} N
              </p>
            </div>
            <div className="metric-card">
              <p className="metric-label">Shear Stress</p>
              <p className="metric-value">
                {latestData.physical_input.shear_stress_kpa.toFixed(2)} kPa
              </p>
            </div>
            <div className="metric-card">
              <p className="metric-label">Motor RPM</p>
              <p className="metric-value">
                {latestData.physical_output.motor_rpm.toFixed(1)}
              </p>
            </div>
          </div>
        )}

      <div className="chart-grid">
        <div className="chart-container">
          <h3 className="chart-title">Shear Force & Friction</h3>
          <UPlotChart data={shearForceData} options={shearForceOptions} />
        </div>
        <div className="chart-container">
          <h3 className="chart-title">Vertical Force</h3>
          <UPlotChart data={verticalForceData} options={verticalForceOptions} />
        </div>
        <div className="chart-container">
          <h3 className="chart-title">Shear Stress & Shear Disp.</h3>
          <UPlotChart data={shearStressData} options={shearStressOptions} />
        </div>
        <div className="chart-container">
          <h3 className="chart-title">Vertical Stress & Normal Disp.</h3>
          <UPlotChart
            data={verticalStressData}
            options={verticalStressOptions}
          />
        </div>
        <div className="chart-container">
          <h3 className="chart-title">Front/Rear Vertical Disp. & Tilt</h3>
          <UPlotChart
            data={verticalDispTiltData}
            options={verticalDispTiltOptions}
          />
        </div>
        <div className="chart-container">
          <h3 className="chart-title">EP (kPa)</h3>
          <UPlotChart data={epData} options={epOptions} />
        </div>
        <div className="chart-container">
          <h3 className="chart-title">Motor RPM</h3>
          <UPlotChart data={rpmData} options={rpmOptions} />
        </div>
      </div>
    </main>
  );
};

const root = createRoot(document.getElementById("root")!);
root.render(<App />);
