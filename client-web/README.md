# Client Web

Flask dashboard for local Kernova-TEE build, test, runtime and system status
operations. When the Linux driver is loaded, the validation snapshot reports
`/dev/kernova` and the C++ service selects the kernel-backed path.

```bash
pip install -r client-web/requirements.txt
python3 client-web/app.py
```

The dashboard is also available through the `web` service in
`infra/compose.yaml`.
