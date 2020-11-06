#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

import urllib.request
import json
import os
import logging
import time

_GITHUB_RUN_ID = os.getenv('GITHUB_RUN_ID')
_GITHUB_TOKEN = os.getenv('GITHUB_TOKEN')
_HEADERS = {'Accept': 'application/vnd.github.v3+json', 'Authorization': f'Bearer {_GITHUB_TOKEN}'}


def _main():
    logging.getLogger().setLevel(logging.DEBUG)

    artifact_count = len(os.listdir('coverage/'))
    logging.info('Found %d artifacts')

    while True:
        url = f'https://api.github.com/repos/openthread/openthread/actions/runs/{_GITHUB_RUN_ID}/artifacts'
        logging.info('GET %s', url)
        with urllib.request.urlopen(urllib.request.Request(
                url=url,
                headers=_HEADERS,
        )) as response:
            result = json.loads(response.read().decode())
            logging.debug("Result: %s", result)

            artifacts = result['artifacts']
            if len(artifacts) < artifact_count:
                logging.info('Waiting for artifacts...')
                time.sleep(10)
                continue

            for artifact in artifacts:
                if not artifact['name'].startswith('cov-'):
                    continue

                artifact_id = artifact['id']
                url = f'https://api.github.com/repos/openthread/openthread/actions/artifacts/{artifact_id}'
                logging.info('DELETE %s', url)
                with urllib.request.urlopen(urllib.request.Request(
                        url=url,
                        method='DELETE',
                )):
                    pass

            break


if __name__ == "__main__":
    _main()
