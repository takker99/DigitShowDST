import { type FunctionComponent, useEffect, useRef, useState } from "react";
import uPlot from "uplot";

type ChartData = number[][];
type ChartOptions = Omit<uPlot.Options, "width" | "height">;

export const UPlotChart: FunctionComponent<{
  data: ChartData;
  options: ChartOptions;
}> = ({ data, options }) => {
  const containerRef = useRef<HTMLDivElement>(null);
  const plotRef = useRef<uPlot>(null);
  const [size, setSize] = useState({ width: 0, height: 0 });

  useEffect(() => {
    if (!containerRef.current) return;

    const updateSize = () => {
      if (containerRef.current) {
        const rect = containerRef.current.getBoundingClientRect();
        setSize({ width: rect.width, height: 300 });
      }
    };

    updateSize();
    const resizeObserver = new ResizeObserver(updateSize);
    resizeObserver.observe(containerRef.current);
    return () => resizeObserver.disconnect();
  }, []);

  useEffect(() => {
    if (!containerRef.current || size.width === 0) return;

    if (plotRef.current) {
      plotRef.current.destroy();
      plotRef.current = null;
    }

    const opts: uPlot.Options = {
      ...options,
      width: size.width,
      height: size.height,
    };
    plotRef.current = new uPlot(
      opts,
      data as unknown as uPlot.AlignedData,
      containerRef.current,
    );

    return () => {
      if (plotRef.current) {
        plotRef.current.destroy();
        plotRef.current = null;
      }
    };
  }, [options, size]);

  useEffect(() => {
    if (plotRef.current && data) {
      plotRef.current.setData(data as unknown as uPlot.AlignedData);
    }
  }, [data]);

  return <div ref={containerRef} />;
};
