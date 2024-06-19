node {
    try {
        stage('Setup') {
            println("Setup stage!")
        }
        stage("Build") {
            println("Build stage!")
            sh 'make'
        }
        stage("Test") {
            println("Test Stage!")
            sh 'make grade'
        }
        stage("Analyze"){
            println("Analyze Stage!")
        }
        post {
            always {
                sh 'make clean' // Clean up resources
            }
        }
    } catch (Exception e) {
        currentBuild.result = 'FAILURE'
        echo "Error: ${e.message}"
    }
}
